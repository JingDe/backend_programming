
#include<ngx_http.h>
#include<ngx_core.h>
#include<ngx_config.h>
#include<nginx.h>
#include<ngx_event.h>
#include<time.h>

static char* ngx_http_mytest(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);
static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r);


// 模块接口
static ngx_http_module_t ngx_http_mytest_module_ctx={
	NULL, 
	NULL,
	NULL, 
	NULL,
	NULL, 
	NULL,
	NULL, 
	NULL
};

// mytest模块关心的配置项
static ngx_command_t ngx_http_mytest_commands[]={
	{
		ngx_string("mytest"),
		NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,
		ngx_http_mytest,
		NGX_HTTP_LOC_CONF_OFFSET, // ??
		0,
		NULL
	},
	ngx_null_command	
};

ngx_module_t ngx_http_mytest_module={
	NGX_MODULE_V1,
	
	&ngx_http_mytest_module_ctx, // 模块接口
	ngx_http_mytest_commands,
	NGX_HTTP_MODULE, // 模块类型
	
	NULL,
	NULL, /* nginx启动时，worker启动前，调用各个模块的初始化回调方法，mytest模块不需要*/
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	
	NGX_MODULE_V1_PADDING
};


// 配置项mytest解析方法
static char* ngx_http_mytest(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
	ngx_http_core_loc_conf_t *clcf;
	clcf = (ngx_http_core_loc_conf_t*)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
		/* clcf找到mytest配置项所属的配置快， */
		
	clcf->handler = ngx_http_mytest_handler; 
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

static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r)
{
	if(!(r->method  &  (NGX_HTTP_GET | NGX_HTTP_HEAD)))
		return NGX_HTTP_NOT_ALLOWED;
	
	// 丢弃包体
	ngx_int_t rc=ngx_http_discard_request_body(r);
	if(rc != NGX_OK)
		return rc;
	
	ngx_str_t type = ngx_string("text/plain");
	ngx_str_t response=ngx_string("test test test");
	
	r->headers_out.status=NGX_HTTP_OK;
	r->headers_out.content_length_n = response.len;
	r->headers_out.content_type = type;
	
	rc=ngx_http_send_header(r);
	if(rc==NGX_ERROR  ||  rc>NGX_OK  ||  r->header_only)
		return rc;
	
	ngx_buf_t *b;
	b=ngx_create_temp_buf(r->pool, response.len);
	if(b==NULL)
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	
	ngx_memcpy(b->pos, response.data, response.len);
	b->last=b->pos + response.len;
	b->last_buf=1;
	
	ngx_chain_t out;
	out.buf=b;
	out.next=NULL;
	return ngx_http_output_filter(r, &out);
}