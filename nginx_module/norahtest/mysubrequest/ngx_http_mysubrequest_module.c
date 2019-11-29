#include<ngx_http.h>
#include<nginx.h>

static char* ngx_http_mysubrequest(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);
static ngx_int_t ngx_http_mysubrequest_handler(ngx_http_request_t *r);
static void mysubrequest_post_handler(ngx_http_request_t *r);


// 请求上下文
typedef struct{
	ngx_str_t stock[6]; // 保存子请求回调方法中解析出来的股票数据
}ngx_http_mysubrequest_ctx_t;

// http://hq.sinajs.cn/list=s_sh000001
// var hq_str_s_sh000001="上证指数,2906.1688,20.8804,0.72,1785874,17423458";



// 模块接口
static ngx_http_module_t ngx_http_mysubrequest_module_ctx={
	NULL, 
	NULL,
	NULL, 
	NULL,
	NULL, 
	NULL,
	NULL, 
	NULL
};

// mysubrequest模块关心的配置项
static ngx_command_t ngx_http_mysubrequest_commands[]={
	{
		ngx_string("mysubrequest"),
		NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,
		ngx_http_mysubrequest,
		NGX_HTTP_LOC_CONF_OFFSET, // ??
		0,
		NULL
	},
	ngx_null_command	
};

ngx_module_t ngx_http_mysubrequest_module={
	NGX_MODULE_V1,
	
	&ngx_http_mysubrequest_module_ctx, // 模块接口
	ngx_http_mysubrequest_commands,
	NGX_HTTP_MODULE, // 模块类型
	
	NULL,
	NULL, /* nginx启动时，worker启动前，调用各个模块的初始化回调方法，mysubrequest模块不需要*/
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	
	NGX_MODULE_V1_PADDING
};




// 子请求结束时的回调方法
// 参数r是子请求，参数rc是子请求在结束时的状态，这里是http响应码，例如200.
static ngx_int_t mysubrequest_subrequest_post_handler(ngx_http_request_t *r, void* data, ngx_int_t rc)
{
	ngx_http_request_t *pr=r->parent;
	
	/*上下文保存在父请求中。参数data就是上下文，初始化subrequest时进行设置。*/
	ngx_http_mysubrequest_ctx_t *myctx=ngx_http_get_module_ctx(pr, ngx_http_mysubrequest_module);
	
	pr->headers_out.status = r->headers_out.status;
	if(r->headers_out.status == NGX_HTTP_OK)
	{
		// 访问成功，解析包体
		int flag=0;
		/*在不转发响应时，buffer中保存上游服务器的响应。
		在使用反向代理模块访问上游服务器时，如果使用upstream机制时没有重定义input_filter方法，
		upstream机制默认的input_filter方法会试图把所有上游响应全部保存到buffer缓冲区中。*/
		ngx_buf_t* pRecvBuf = &r->upstream->buffer;
		
		for(; pRecvBuf->pos!=pRecvBuf->last; pRecvBuf->pos++)
		{
			if(*pRecvBuf->pos == ','  ||  *pRecvBuf->pos=='\"')
			{
				if(flag>0)
				{
					myctx->stock[flag-1].len=pRecvBuf->pos - myctx->stock[flag-1].data;
				}
				flag++;
				myctx->stock[flag-1].data = pRecvBuf->pos+1;
			}
			if(flag>6)
				break;
		}
	}
	// 设置父请求的回调方法
	pr->write_event_handler=mysubrequest_post_handler;
	return NGX_OK;
}

// 父请求的回调方法
static void mysubrequest_post_handler(ngx_http_request_t *r)
{
	if(r->headers_out.status != NGX_HTTP_OK)
	{
		ngx_http_finalize_request(r, r->headers_out.status);
		return;
	}
	
	ngx_http_mysubrequest_ctx_t* myctx=ngx_http_get_module_ctx(r, ngx_http_mysubrequest_module);
	
	ngx_str_t output_format = ngx_string("stock[%V], Today current price: %V, volumn: %V");
	
	int bodylen =output_format.len + myctx->stock[0].len +myctx->stock[1].len + myctx->stock[4].len-6;
	r->headers_out.content_length_n =bodylen;
	
	ngx_buf_t* b=ngx_create_temp_buf(r->pool, bodylen);
	ngx_snprintf(b->pos, bodylen, (char*)output_format.data, &myctx->stock[0], &myctx->stock[1], &myctx->stock[4]);
	b->last=b->pos+bodylen;
	b->last_buf=1;
	
	ngx_chain_t out;
	out.buf=b;
	out.next=NULL;
	
	static ngx_str_t type=ngx_string("text/plain; charset=GBK");
	r->headers_out.content_type=type;
	r->headers_out.status=NGX_HTTP_OK;
	
	r->connection->buffered |= NGX_HTTP_WRITE_BUFFERED;
	ngx_int_t ret= ngx_http_send_header(r);
	ret =ngx_http_output_filter(r, &out);
	
	ngx_http_finalize_request(r, ret);	
}

// 启动subrequest
static ngx_int_t ngx_http_mysubrequest_handler(ngx_http_request_t *r)
{
	ngx_http_mysubrequest_ctx_t* myctx=ngx_http_get_module_ctx(r, ngx_http_mysubrequest_module);
	if(myctx==NULL)
	{
		myctx = ngx_palloc(r->pool, sizeof(ngx_http_mysubrequest_ctx_t));
		if(myctx ==NULL)
			return NGX_ERROR;
		ngx_http_set_ctx(r, myctx, ngx_http_mysubrequest_module);
	}
	
	ngx_http_post_subrequest_t *psr=ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
	if(psr==NULL)
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	// 设置子请求回调方法
	psr->handler =mysubrequest_subrequest_post_handler;
	
	// 将data设为myctx上下文，回调mysubreqest_subrequest_post_handler时，参数data就是myctx
	psr->data=myctx;
	
	ngx_str_t sub_prefix =ngx_string("/list=");
	ngx_str_t sub_location;
	sub_location.len = sub_prefix.len + r->args.len;
	sub_location.data =ngx_palloc(r->pool, sub_location.len);
	ngx_snprintf(sub_location.data, sub_location.len, "%V%V", &sub_prefix, &r->args);
	
	ngx_http_request_t* sr;
	/* 创建子请求，返回NGX_OK或者NGX_ERROR。
	NGX_HTTP_SUBREQUEST_IN_MEMROY告诉upstream模块，把上游服务器的全部响应全部保存在子请求的sr->upstream->buffer内存缓冲区中 */
	ngx_int_t rc=ngx_http_subrequest(r, &sub_location, NULL, &sr, psr, NGX_HTTP_SUBREQUEST_IN_MEMORY);
	if(rc != NGX_OK)
		return NGX_ERROR;
	
	return NGX_DONE;
}



// 配置项mysubrequest解析方法
static char* ngx_http_mysubrequest(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
	ngx_http_core_loc_conf_t *clcf;
	clcf = (ngx_http_core_loc_conf_t*)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
		/* clcf找到mysubrequest配置项所属的配置快， */
		
	clcf->handler = ngx_http_mysubrequest_handler; 
	/* 设置请求的处理方法， 在NGX_HTTP_CONTENT_PHASE阶段处理。
		http框架接收完http请求头部时，调用handler方法。*/
	return NGX_CONF_OK;
}

