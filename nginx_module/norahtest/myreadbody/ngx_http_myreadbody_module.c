#include<ngx_http.h>
#include<nginx.h>

static char* ngx_http_myreadbody(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);
static ngx_int_t ngx_http_myreadbody_handler(ngx_http_request_t *r);
static void ngx_http_myreadbody_body_handler(ngx_http_request_t *r);

// 模块接口
static ngx_http_module_t ngx_http_myreadbody_module_ctx={
	NULL, 
	NULL,
	NULL, 
	NULL,
	NULL, 
	NULL,
	NULL, 
	NULL
};

// myreadbody模块关心的配置项
static ngx_command_t ngx_http_myreadbody_commands[]={
	{
		ngx_string("myreadbody"),
		NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,
		ngx_http_myreadbody,
		NGX_HTTP_LOC_CONF_OFFSET, // ??
		0,
		NULL
	},
	ngx_null_command	
};

ngx_module_t ngx_http_myreadbody_module={
	NGX_MODULE_V1,
	
	&ngx_http_myreadbody_module_ctx, // 模块接口
	ngx_http_myreadbody_commands,
	NGX_HTTP_MODULE, // 模块类型
	
	NULL,
	NULL, /* nginx启动时，worker启动前，调用各个模块的初始化回调方法，myreadbody模块不需要*/
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	
	NGX_MODULE_V1_PADDING
};


// 配置项myreadbody解析方法
static char* ngx_http_myreadbody(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
	ngx_http_core_loc_conf_t *clcf;
	clcf = (ngx_http_core_loc_conf_t*)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
		/* clcf找到myreadbody配置项所属的配置快， */
		
	clcf->handler = ngx_http_myreadbody_handler; 
	/* 设置请求的处理方法， 在NGX_HTTP_CONTENT_PHASE阶段处理。
		http框架接收完http请求头部时，调用handler方法。*/
	return NGX_CONF_OK;
}
/*
#define ngx_http_conf_get_module_loc_conf(cf, module)                         \                              
    ((ngx_http_conf_ctx_t *) cf->ctx)->loc_conf[module.ctx_index]

typedef struct {
    void        **main_conf;
    void        **srv_conf;
    void        **loc_conf;
} ngx_http_conf_ctx_t;

*/



static ngx_int_t ngx_http_myreadbody_handler(ngx_http_request_t *r)
{
	if(!(r->method  &  (NGX_HTTP_GET | NGX_HTTP_HEAD  |  NGX_HTTP_POST)))
		return NGX_HTTP_NOT_ALLOWED;
	
/* 	// 丢弃包体
	ngx_int_t rc=ngx_http_discard_request_body(r);
	if(rc != NGX_OK)
		return rc; */
	ngx_int_t rc=ngx_http_read_client_request_body(r, ngx_http_myreadbody_body_handler);
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "ngx_http_read_client_request_body return %d", rc);
	if(rc >= NGX_HTTP_SPECIAL_RESPONSE)
	{
		return rc;
	}
	return NGX_DONE;
	/*
	NGX_DONE： 表示到此为止， 同时HTTP框架将暂时不再继续执行这个请求的后续部分。 事实上， 这时会检查连接的类型， 如果是keepalive类型的用户请求， 就会保持住HTTP连接， 然后把控制权交给Nginx。 
	*/
	
	/*
	return ngx_http_output_filter(r, &out); // 将ngx_http_output_filter发送包体的结果传递给 ngx_http_finalize_request 方法
	*/
}


/* 接收包体回调方法：
	typedef void (*ngx_http_client_body_handler_pt)(ngx_http_request_t *r);
	
	如果 r->request_body_in_file_only 为1，可以在r->request_body->temp_file->file中获取临时文件/包体。
	r->request_body->temp_file->file.name获取到文件路径。
 */
static void ngx_http_myreadbody_body_handler(ngx_http_request_t *r)
{	
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "rest = %d, to_write = %d", r->request_body->rest, r->request_body->to_write);
	if(r->request_body->bufs) // 接收包体的 ngx_buf_t链表
	{
		ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->request_body->bufs not NULL");
		
		ngx_chain_t *bufNode=r->request_body->bufs;
		while(bufNode)
		{
			ngx_buf_t *bodyBuf=bufNode->buf;
			if(bodyBuf)
			{
				if(bodyBuf->in_file == 0)
				{
					ngx_str_t body={bodyBuf->last - bodyBuf->pos, (u_char*)bodyBuf->pos};
					ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "%V", &(body));
				}				
				else
				{
					// if(bodyBuf->file)
				}
			}
			else
				break;
			bufNode=bufNode->next;
		}
	}
	else
	{
		ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "ERROR r->request_body->bufs is NULL");
	}
	
	ngx_str_t type = ngx_string("text/plain");
	ngx_str_t response=ngx_string("my readbody");
	
	r->headers_out.status=NGX_HTTP_OK;
	r->headers_out.content_length_n = response.len;
	r->headers_out.content_type = type;
	
	ngx_int_t rc=ngx_http_send_header(r);
	if(rc==NGX_ERROR  ||  rc>NGX_OK  ||  r->header_only)
	{
		ngx_http_finalize_request(r, rc);
	}
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "ngx_http_send_header ok");
	
	ngx_buf_t *b;
	b=ngx_create_temp_buf(r->pool, response.len);
	if(b==NULL)
	{
		ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
		//return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	
	ngx_memcpy(b->pos, response.data, response.len);
	b->last=b->pos + response.len;
	b->last_buf=1;
	
	ngx_chain_t out;
	out.buf=b;
	out.next=NULL;
	rc = ngx_http_output_filter(r, &out);
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "ngx_http_output_filter return %d", rc);
	ngx_http_finalize_request(r, rc);
	//ngx_http_finalize_request(r, NGX_OK);
}
