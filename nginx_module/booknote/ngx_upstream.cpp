
typedef struct ngx_http_upstream_s ngx_http_upstream_t;

struct ngx_http_upstream_s{
	
	/*request_bufs决定发送什么样的请求给上游服务器，在实现create_request方法时需要设置*/
	ngx_chain_t *request_bufs;
	
	/*upstream访问时的所有限制性参数*/
	ngx_http_upstream_conf_t *conf;
	
	/*通过resolved可以直接指定上游服务器地址*/
	ngx_http_upstream_resolved_t *resolved;
	
	/*存储接收自上游服务器发来的响应内容。
	1，在使用process_header方法解析上游响应的包头时，buffer保存完整的响应包头；
	2，当buffering为1，此时upstream是向下游转发上游的包体时，buffer没有意义；
	3，当buffering为0，buffer用于反复接收上游的包体，向下游转发；
	4，当upstream并不用于转发上游包体时，buffer会被用于反复接收上游的包体*/
	ngx_buf_t buffer;
	
	// 构造发往上游服务器的请求内容
	ngx_int_t (*create_request)(ngx_http_request_t *r);
	
	/*收到上游服务器的响应后回调process_header。
	返回NGX_AGAIN表示未收到完整包头。*/
	ngx_int_t (*process_header)(ngx_http_request_t *r);
	
	// 销毁upstream请求时调用
	void (*finalize_request)(ngx_http_request_t *r, ngx_int_t rc);
	
	/*在向客户端转发上游服务器的包体时才有用。
	当buffring为1，表示使用多个缓冲区以及磁盘文件来转发上游的响应包体。
	当buffering为0，表示只使用上面的buffer向下游转发响应包体。*/
	unsigned buffering:1;
};

static ngx_command_t ngx_http_myupstream_commands[]={
	{
		ngx_string("upstream_connn_timeout"),
		NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
		ngx_conf_set_msec_slot,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_myupstream_conf_t, upstream.conn_timeout),
		NULL
	},
	
};

ngx_http_myupstream_handler()
{
	
	ngx_http_myupstream_conf_t *mycf=(ngx_http_myupstream_conf_t *) ngx_http_get_module_loc_conf(r, ngx_http_myupstream_module);
	r->upstream->conf = &mycf->upstream;
}

