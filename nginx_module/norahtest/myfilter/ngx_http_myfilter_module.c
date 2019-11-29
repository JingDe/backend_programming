
#include<ngx_http.h>
#include<nginx.h>



static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt ngx_http_next_body_filter;

// 响应包体前缀字符串
static ngx_str_t filter_prefix= ngx_string("[my filter prefix]");

static ngx_int_t ngx_http_myfilter_header_filter(ngx_http_request_t *r);
static ngx_int_t ngx_http_myfilter_body_filter(ngx_http_request_t *r, ngx_chain_t *in);
static ngx_int_t ngx_http_myfilter_init(ngx_conf_t* cf);

/*
存储配置项的结构体
*/
typedef struct{
	ngx_flag_t enable; // 解析配置文件，决定是否开启配置模块
}ngx_http_myfilter_conf_t;

/*分配存储配置项的结构体*/
static void* ngx_http_myfilter_create_conf(ngx_conf_t *cf)
{
	ngx_http_myfilter_conf_t *mycf;
	mycf = (ngx_http_myfilter_conf_t*)ngx_palloc(cf->pool, sizeof(ngx_http_myfilter_conf_t));
	if(mycf==NULL)
		return NULL;
	
	mycf->enable=NGX_CONF_UNSET;/*为了使用预设函数ngx_conf_set_flag_slot解析配置项参数，初始化ngx_flat_t类型变量*/
	return mycf;
}

/*配置项合并方法，*/
static char* ngx_http_myfilter_merge_conf(ngx_conf_t *cf, void* parent, void* child)
{
	ngx_http_myfilter_conf_t *prev= (ngx_http_myfilter_conf_t*)parent;
	ngx_http_myfilter_conf_t *conf= (ngx_http_myfilter_conf_t*)child;
	
	ngx_conf_merge_value(conf->enable, prev->enable, 0);
	return NGX_CONF_OK;
}

/*http请求你上下文*/
typedef struct{
	ngx_int_t add_prefix; /*为0表示不需要在返回的包体之前添加前缀，为1表示在包体之前加前缀，
		为2表示已经添加过前缀了。*/
}ngx_http_myfilter_ctx_t;



// 模块接口
static ngx_http_module_t ngx_http_myfilter_module_ctx={
	NULL, // preconfiguration方法
	ngx_http_myfilter_init,  /*postconfiguration方法，模块初始化方法*/ // TODO
	NULL, // create_main_conf
	NULL, // init_main_conf
	NULL, // create_srv_conf方法
	NULL, // merge_srv_conf方法
	ngx_http_myfilter_create_conf, // create_loc_conf方法
	ngx_http_myfilter_merge_conf // merge_loc_conf方法
};

// myfilter模块关心的配置项
static ngx_command_t ngx_http_myfilter_commands[]={
	{
		ngx_string("add_prefix"),
		NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_FLAG,
		ngx_conf_set_flag_slot,
		NGX_HTTP_LOC_CONF_OFFSET, // ??
		offsetof(ngx_http_myfilter_conf_t, enable), // 存储解析配置项的结果到ngx_http_myfilter_conf_t的enable成员
		NULL
	},
	ngx_null_command	
};

ngx_module_t ngx_http_myfilter_module={
	NGX_MODULE_V1,
	
	&ngx_http_myfilter_module_ctx, // 模块接口,模块上下文
	ngx_http_myfilter_commands,
	NGX_HTTP_MODULE, // 模块类型仍然是http类型
	
	NULL,
	NULL, /* nginx启动时，worker启动前，调用各个模块的初始化回调方法，myfilter模块不需要*/
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	
	NGX_MODULE_V1_PADDING
};


/*初始化http过滤模块*/
static ngx_int_t ngx_http_myfilter_init(ngx_conf_t* cf)
{
	ngx_http_next_header_filter = ngx_http_top_header_filter;
	ngx_http_top_header_filter = ngx_http_myfilter_header_filter;
	
	ngx_http_next_body_filter = ngx_http_top_body_filter;
	ngx_http_top_body_filter = ngx_http_myfilter_body_filter;
	
	return NGX_OK;
}



static ngx_int_t ngx_http_myfilter_header_filter(ngx_http_request_t *r)
{
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "in ngx_http_myfilter_header_filter");
	ngx_http_myfilter_ctx_t *ctx;
	ngx_http_myfilter_conf_t *conf;
	
	// 仅当响应码为200的时候，添加前缀
	if(r->headers_out.status != NGX_HTTP_OK)
	{
		return ngx_http_next_header_filter(r);
	}
	
	// 获得当前模块的上下文
	ctx = ngx_http_get_module_ctx(r, ngx_http_myfilter_module);
	if(ctx)
	{
		// 请求上下文已经存在，说明ngx_http_myfilter_header_filter已经被调用过
		return ngx_http_next_header_filter(r);
	}
	
	/*获取存储配置项的ngx_http_myfilter_conf_t结构体*/
	conf = ngx_http_get_module_loc_conf(r, ngx_http_myfilter_module);
	if(conf->enable == 0)
	{
		return ngx_http_next_header_filter(r);
	}
	
	ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_myfilter_ctx_t));
	if(ctx == NULL)
	{
		return NGX_ERROR;
	}
	
	ctx->add_prefix = 0;
	// 将请求上下文ctx添加到当前请求
	ngx_http_set_ctx(r, ctx, ngx_http_myfilter_module);
	
	// 仅处理plain类型的响应包体
	// if(r->headers_out.content_type.len >= sizeof("text/plain")-1  &&
		// ngx_strncasecmp(r->headers_out.content_type.data, (u_char*)"text/plain", sizeof("text/plain")-1)==0)
	{
		ctx->add_prefix = 1;
		
		if(r->headers_out.content_length_n > 0)
			r->headers_out.content_length_n += filter_prefix.len;
	}
	
	return ngx_http_next_header_filter(r);
}

static ngx_int_t ngx_http_myfilter_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "in ngx_http_myfilter_body_filter");
	ngx_http_myfilter_ctx_t *ctx;
	ctx = ngx_http_get_module_ctx(r, ngx_http_myfilter_module);
	
	if(ctx==NULL  ||  ctx->add_prefix!=1)
	{
		return ngx_http_next_body_filter(r, in);
	}
	
	ctx->add_prefix=2;
	ngx_buf_t *b=ngx_create_temp_buf(r->pool, filter_prefix.len);
	b->start=b->pos=filter_prefix.data;
	b->last=b->pos+filter_prefix.len;
	
	/*从请求内存池中生成ngx_chain_t链表，将b设置到buf成员中。
	并添加到原先待发送的http包体前面*/
	ngx_chain_t *cl=ngx_alloc_chain_link(r->pool);
	cl->buf=b;
	cl->next=in;
	
	return ngx_http_next_body_filter(r, cl);
}




