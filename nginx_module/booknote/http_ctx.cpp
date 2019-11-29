
/*
使用上下文结构体
*/

// 上下文结构体
typedef struct{
	ngx_uint_t my_step;
} ngx_http_mytest_ctx_t;

static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r)
{
	
	ngx_http_mytest_ctx_t* myctx = ngx_http_get_module_ctx(r, ngx_http_mytest_module);
	if(myctx==NULL)
	{
		myctx = ngx_pcalloc(r->pool, sizeof(ngx_http_mytest_ctx_t));
		if(myctx == NULL)
			return NGX_ERROR;
		
		ngx_http_set_ctx(r, myctx, ngx_http_mytest_module);
	}
	
	
}

