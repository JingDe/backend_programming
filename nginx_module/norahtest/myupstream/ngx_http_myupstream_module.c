
#include<ngx_http.h>
#include<ngx_core.h>
#include<ngx_config.h>
#include<nginx.h>
#include<ngx_event.h>
#include<time.h>

static char* ngx_http_myupstream(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);
static ngx_int_t ngx_http_myupstream_handler(ngx_http_request_t *r);
static void printHostent(struct hostent* host);
static ngx_int_t myupstream_upstream_process_header(ngx_http_request_t *r);

// ngx_http_proxy_module.c
//static ngx_str_t  ngx_http_proxy_hide_headers;
static ngx_str_t  ngx_http_proxy_hide_headers[] =
{
    ngx_string("Date"),
    ngx_string("Server"),
    ngx_string("X-Pad"),
    ngx_string("X-Accel-Expires"),
    ngx_string("X-Accel-Redirect"),
    ngx_string("X-Accel-Limit-Rate"),
    ngx_string("X-Accel-Buffering"),
    ngx_string("X-Accel-Charset"),
    ngx_null_string
};


/*配置结构体*/
typedef struct{
	ngx_http_upstream_conf_t upstream; // 所有请求共享同一个ngx_http_upstream_conf_t结构体
	// 启动upstream之前，赋给r->upstream->conf
} ngx_http_myupstream_conf_t;

static void* ngx_http_myupstream_create_loc_conf(ngx_conf_t *cf)
{
	ngx_http_myupstream_conf_t *mycf;
	mycf = (ngx_http_myupstream_conf_t*) ngx_pcalloc(cf->pool, sizeof(ngx_http_myupstream_conf_t));
	if(mycf == NULL)
		return NULL;
	
	mycf->upstream.connect_timeout=60000; // 1分钟
	mycf->upstream.send_timeout=6000;
	mycf->upstream.read_timeout=6000;
	mycf->upstream.store_access=600;
	
	/*以固定大小内存作为缓冲区转发上游的响应包体，大小为buffer_size*/
	mycf->upstream.buffering=0;
	mycf->upstream.bufs.num=8; 
	/*如果buffering为1，最多使用bufs.nums个缓冲区，
		且每个缓冲区大小为bufs.size。另外使用临时文件，最大长度为max_temp_file_size*/
	mycf->upstream.bufs.size=ngx_pagesize;
	mycf->upstream.buffer_size=ngx_pagesize;
	mycf->upstream.busy_buffers_size=2*ngx_pagesize;
	mycf->upstream.temp_file_write_size=2*ngx_pagesize;
	mycf->upstream.max_temp_file_size=1024*1024*1024;
	
	/*ngx_http_upstream_process_headers方法按照hide_headers将本应转发给下游的一些http头部隐藏。
		在merge合并配置项方法中使用upstream提供的ngx_http_upstream_hide_headers_hash*/
	mycf->upstream.hide_headers=NGX_CONF_UNSET_PTR;
	mycf->upstream.pass_headers=NGX_CONF_UNSET_PTR;
	return mycf;
}

static char* ngx_http_myupstream_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child)
{
	ngx_http_myupstream_conf_t *prev=(ngx_http_myupstream_conf_t*)parent;
	ngx_http_myupstream_conf_t *conf=(ngx_http_myupstream_conf_t*)child;
	
	ngx_hash_init_t hash;
	hash.max_size=100;
	hash.bucket_size=1024;
	if(ngx_http_upstream_hide_headers_hash(cf, &conf->upstream, &prev->upstream, ngx_http_proxy_hide_headers, &hash)!=NGX_OK)
	{
		return NGX_CONF_ERROR;
	}
	return NGX_CONF_OK;
}

// 请求上下文
typedef struct{
	ngx_http_status_t status;
}ngx_http_myupstream_ctx_t;

// typedef struct{
	// ngx_uint_t code;
	// ngx_uint_t count;
	// u_char *start;
	// u_char *end;
// }ngx_http_status_t;


// myupstream模块关心的配置项
static ngx_command_t ngx_http_myupstream_commands[]={
	{
		ngx_string("myupstream"),
		NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,
		ngx_http_myupstream,
		NGX_HTTP_LOC_CONF_OFFSET, // ??
		0,
		NULL
	},
	ngx_null_command	
};


// 模块接口
static ngx_http_module_t ngx_http_myupstream_module_ctx={
	NULL, 
	NULL,
	NULL, 
	NULL,
	NULL, 
	NULL,
	ngx_http_myupstream_create_loc_conf, 
	ngx_http_myupstream_merge_loc_conf
};

ngx_module_t ngx_http_myupstream_module={
	NGX_MODULE_V1,
	
	&ngx_http_myupstream_module_ctx, // 模块接口
	ngx_http_myupstream_commands,
	NGX_HTTP_MODULE, // 模块类型
	
	NULL,
	NULL, /* nginx启动时，worker启动前，调用各个模块的初始化回调方法，myupstream模块不需要*/
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	
	NGX_MODULE_V1_PADDING
};

// 构造请求
static ngx_int_t myupstream_upstream_create_request(ngx_http_request_t *r)
{
	// 请求url：/searchqq=...
	static ngx_str_t backendQueryLine = ngx_string("GET /search?q=%V HTTP/1.1\r\nHost: www.baidu.com\r\nConnection: close\r\n\r\n");
	ngx_int_t queryLineLen = backendQueryLine.len+r->args.len-2; // ???
	
	ngx_buf_t* b=ngx_create_temp_buf(r->pool, queryLineLen);
	if(b==NULL)
		return NGX_ERROR;
	
	b->last=b->pos+queryLineLen;
	
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->args is %V", &(r->args));
	ngx_snprintf(b->pos, queryLineLen, (char*)backendQueryLine.data, &r->args);
	
	/*r->upstream->request_bufs包含要发送给上游服务器的请求*/
	r->upstream->request_bufs = ngx_alloc_chain_link(r->pool);
	if(r->upstream->request_bufs ==NULL)
		return NGX_ERROR;
	
	r->upstream->request_bufs->buf =b;
	r->upstream->request_bufs->next =NULL;
	r->upstream->request_sent =0;
	r->upstream->header_sent=0;
	r->header_hash=1;
	return NGX_OK;
}

// 解析包头, 就是解析http响应行和http头部
// 解析http响应行
static ngx_int_t myupstream_process_status_line(ngx_http_request_t *r)
{
	size_t len;
	ngx_int_t rc;
	ngx_http_upstream_t *u;
	
	// 请求上下文保存多次解析http响应行的状态
	ngx_http_myupstream_ctx_t *ctx=ngx_http_get_module_ctx(r, ngx_http_myupstream_module);
	if(ctx==NULL)
		return NGX_ERROR;
	
	u=r->upstream;
	
	// 调用http框架的ngx_http_parse_status_line方法，解析http响应行，输入是收到的字节流，和上下文中的ngx_http_status_t结构
	rc=ngx_http_parse_status_line(r, &u->buffer, &ctx->status);
	if(rc==NGX_AGAIN)
		return rc;
	
	if(rc==NGX_ERROR)
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "upstream sent no valid HTTP/1.0 header");
		r->http_version = NGX_HTTP_VERSION_9;
		u->state->status=NGX_HTTP_OK;
		return NGX_OK;
	}
	
	/*将解析出的信息ctx->status设置到r->upstream->headers_in结构体。
	当解析完成时，再设置到向下游发送的r->headers_out结构体中。*/
	if(u->state)
		u->state->status = ctx->status.code;
	
	u->headers_in.status_n = ctx->status.code;
	
	len=ctx->status.end - ctx->status.start;
	u->headers_in.status_line.len=len;
	
	u->headers_in.status_line.data = ngx_pnalloc(r->pool, len);
	if(u->headers_in.status_line.data == NULL)
		return NGX_ERROR;
	
	ngx_memcpy(u->headers_in.status_line.data, ctx->status.start, len);
	
	/*设置解析http头部的回调方法。*/
	u->process_header = myupstream_upstream_process_header;
	
	return myupstream_upstream_process_header(r);
}

/*解析http响应头部，把上游服务器发送的http头部添加到请求r->upstream->headers_in.headers链表中*/
static ngx_int_t myupstream_upstream_process_header(ngx_http_request_t *r)
{
	ngx_int_t rc;
	ngx_table_elt_t *h;
	ngx_http_upstream_header_t *hh;
	ngx_http_upstream_main_conf_t *umcf;
	
	// upstream模块的配置项
	umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);
	
	for(;;)
	{
		/* 解析http头部， 返回OK表示解析出一行http头部，
		返回NGX_HTTP_PARSE_HEADER_DONE表示所有http头部都解析完毕,
		返回NGX_AGAIN，表示状态机没有解析到完整的http头部。*/
		rc = ngx_http_parse_header_line(r, &r->upstream->buffer, 1);
		if(rc==NGX_OK)
		{
			// 将http头部添加到headers_in.headers链表中
			h= ngx_list_push(&r->upstream->headers_in.headers);
			if(h==NULL)
				return NGX_ERROR;
			
			h->hash = r->header_hash;
			h->key.len = r->header_name_end - r->header_name_start;
			h->value.len = r->header_end -r->header_start;
			h->key.data= ngx_pnalloc(r->pool, h->key.len+1 + h->value.len+1 + h->key.len);
			if(h->key.data ==NULL)
				return NGX_ERROR;
			
			h->value.data =h->key.data +h->key.len+1;
			h->lowcase_key = h->key.data+h->key.len+1 +h->value.len+1;
			
			ngx_memcpy(h->key.data, r->header_name_start, h->key.len);
			h->key.data[h->key.len]='\0';
			ngx_memcpy(h->value.data, r->header_start, h->value.len);
			h->value.data[h->value.len]='\0';
			
			if(h->key.len == r->lowcase_index)
				ngx_memcpy(h->lowcase_key, r->lowcase_header, h->key.len);
			else
				ngx_strlow(h->lowcase_key, h->key.data, h->key.len);
			
			hh = ngx_hash_find(&umcf->headers_in_hash, h->hash, h->lowcase_key, h->key.len);
			if(hh  &&  hh->handler(r, h, hh->offset)!=NGX_OK)
				return NGX_ERROR;
			continue;
		}
		
		if(rc == NGX_HTTP_PARSE_HEADER_DONE)
		{
			if(r->upstream->headers_in.server ==NULL)
			{
				h=ngx_list_push(&r->upstream->headers_in.headers); // ngx_list_t
				if(h==NULL)
					return NGX_ERROR;
				
				h->hash =ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash('s', 'e'), 'r'), 'v'), 'e'), 'r');
				ngx_str_set(&h->key, "Server");
				ngx_str_null(&h->value);
				h->lowcase_key =(u_char*)"server";
			}
			
			if(r->upstream->headers_in.date ==NULL)
			{
				h=ngx_list_push(&r->upstream->headers_in.headers);
				if(h==NULL)
					return NGX_ERROR;
				
				h->hash =ngx_hash(ngx_hash(ngx_hash('d', 'a'), 't'), 'e');
				
				ngx_str_set(&h->key, "Date");
				ngx_str_null(&h->value);
				h->lowcase_key=(u_char*)"date";
			}
			return NGX_OK;
		}
		
		if(rc==NGX_AGAIN)
		{
			return NGX_AGAIN;
		}
		
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "upstream sent invalid header");
		return NGX_HTTP_UPSTREAM_INVALID_HEADER;
	}
}

// 在finalize_request中释放资源
static void myupstream_upstream_finalize_request(ngx_http_request_t *r, ngx_int_t rc)
{
	ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "myupstream_upstream_finalize_request");
}

// 启动upstream
static ngx_int_t ngx_http_myupstream_handler(ngx_http_request_t *r)
{
	ngx_http_myupstream_ctx_t *myctx= ngx_http_get_module_ctx(r, ngx_http_myupstream_module);
	if(myctx==NULL)
	{
		myctx = ngx_palloc(r->pool, sizeof(ngx_http_myupstream_ctx_t));
		if(myctx ==NULL)
		{
			return NGX_ERROR;
		}
		
		ngx_http_set_ctx(r, myctx, ngx_http_myupstream_module);
	}
	
	/*对每一个要使用upstream的请求，调用一次ngx_http_upstream_create方法，将会初始化r->upstream成员*/
	if(ngx_http_upstream_create(r) != NGX_OK)
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ngx_http_upstream_create() failed");
		return NGX_ERROR;
	}
	
	ngx_http_myupstream_conf_t *mycf= (ngx_http_myupstream_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_myupstream_module);
	ngx_http_upstream_t *u=r->upstream;
	
	u->conf = &mycf->upstream;
	u->buffering =mycf->upstream.buffering;
	
	// 保存上游服务器地址
	u->resolved = (ngx_http_upstream_resolved_t*) ngx_pcalloc(r->pool, sizeof(ngx_http_upstream_resolved_t));
	if(u->resolved == NULL)
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ngx_pcalloc resolved error. %s", strerror(errno));
		return NGX_ERROR;
	}
	
	static struct sockaddr_in backendSockAddr;
	struct hostent *pHost = gethostbyname((char*) "www.baidu.com");
	if(pHost==NULL)
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "gethostbyname fail. %s", strerror(errno));
		return NGX_ERROR;
	}
	
	// TODO print pHost
	printHostent(pHost);
	
	backendSockAddr.sin_family =AF_INET;
	backendSockAddr.sin_port = htons((in_port_t) 80); // uint16
	char* pDmsIP = inet_ntoa(*(struct in_addr*)(pHost->h_addr_list[0]));
		// char *inet_ntoa(struct in_addr in);		
	backendSockAddr.sin_addr.s_addr = inet_addr(pDmsIP);  // ?? = pHost->h_addr_list[0]->s_addr; uint32
		// in_addr_t inet_addr(const char *cp);
		
	// myctx->backendServer.data = (u_char*) pDmsIP;
	// myctx->backendServer.len = strlen(pDmsIP);
	
	u->resolved->sockaddr = (struct sockaddr*) &backendSockAddr;
	u->resolved->socklen = sizeof(struct sockaddr_in);
	u->resolved->naddrs =1;
	
	//
	u->create_request= myupstream_upstream_create_request;
	u->process_header = myupstream_process_status_line;
	u->finalize_request =myupstream_upstream_finalize_request;
	
	r->main->count++;
	ngx_http_upstream_init(r);
	return NGX_DONE;
}


static void printHostent(struct hostent* host)
{
	
}






// 配置项myupstream解析方法
static char* ngx_http_myupstream(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
	ngx_http_core_loc_conf_t *clcf;
	clcf = (ngx_http_core_loc_conf_t*)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
		/* clcf找到myupstream配置项所属的配置快， */
		
	clcf->handler = ngx_http_myupstream_handler; 
	/* 设置请求的处理方法， 在NGX_HTTP_CONTENT_PHASE阶段处理。
		http框架接收完http请求头部时，调用handler方法。*/
	return NGX_CONF_OK;
}

