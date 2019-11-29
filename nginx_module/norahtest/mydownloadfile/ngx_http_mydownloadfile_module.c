#include<ngx_http.h>
#include<nginx.h>

static const ngx_int_t MAX_FILENAME_LEN=50;

static char* ngx_http_mydownloadfile(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);
static ngx_int_t ngx_http_mydownloadfile_handler(ngx_http_request_t *r);
static void get_http_headers(ngx_http_request_t *r);

static void getMethodInfo(ngx_http_request_t *r);
//static void getRequestInfo(ngx_http_request_t *r);
static ngx_int_t getRequestInfo(ngx_http_request_t *r, ngx_str_t *fileName);
static void getAbsoluteFileName(ngx_http_request_t *r, ngx_str_t *fileNameStr, u_char filename[]);


// 模块接口
static ngx_http_module_t ngx_http_mydownloadfile_module_ctx={
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
static ngx_command_t ngx_http_mydownloadfile_commands[]={
	{
		ngx_string("mydownloadfile"),
		NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,
		ngx_http_mydownloadfile,
		NGX_HTTP_LOC_CONF_OFFSET, // ??
		0,
		NULL
	},
	ngx_null_command	
};

ngx_module_t ngx_http_mydownloadfile_module={
	NGX_MODULE_V1,
	
	&ngx_http_mydownloadfile_module_ctx, // 模块接口
	ngx_http_mydownloadfile_commands,
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
static char* ngx_http_mydownloadfile(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
	ngx_http_core_loc_conf_t *clcf;
	clcf = (ngx_http_core_loc_conf_t*)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
		/* clcf找到mytest配置项所属的配置快， */
		
	clcf->handler = ngx_http_mydownloadfile_handler; 
	/* 设置请求的处理方法， 在NGX_HTTP_CONTENT_PHASE阶段处理。
		http框架接收完http请求头部时，调用handler方法。*/
	return NGX_CONF_OK;
}

static ngx_int_t getRequestInfo(ngx_http_request_t *r, ngx_str_t *fileName)
{
	// 请求下载文件后缀
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->exten is %V", &(r->exten));
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->uri is %V", &(r->uri));
	
	// URI 从 uri_start 到 uri_end 前一个位置
	u_char uriBuf[r->uri_end-r->uri_start+1];
	ngx_memzero(uriBuf, sizeof(uriBuf)/sizeof(u_char));
	ngx_cpystrn(uriBuf, r->uri_start, r->uri_end-r->uri_start);
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->uri(uri_start to uri_end) is %s", uriBuf);
	
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->unparsed_uri is %V", &(r->unparsed_uri));
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->args is %V", &(r->args));
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->http_version is %d", &(r->http_version));
	
	// request_start 到 request_end
	u_char originRequest[r->request_end-r->request_start+1+1];
	ngx_memzero(originRequest, sizeof(originRequest)/sizeof(u_char));
	ngx_cpystrn(originRequest, r->request_start, r->request_end-r->request_start+1);
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "(request_start to request_end) is %s", originRequest);
	
	// 检查请求url是否是/download/.../.../文件名, 返回文件名 
	ngx_int_t prefixLen=sizeof("/download/")/sizeof(char)-1;
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "prefixLen is %d", prefixLen);
	if(ngx_strncmp(r->uri.data, "/download/", prefixLen) == 0)
	{
		// fileName=ngx_pcalloc(r->pool, sizeof(ngx_str_t));
		// if(fileName==0)
			// return -1;
		fileName->data=r->uri_start+prefixLen;
		fileName->len=(r->uri_end-r->uri_start) - prefixLen;
		ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "fileName is %V", (fileName));
		return 0;
	}
	else
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "uri error, %V, %s, %d", &(r->uri), "/download/", prefixLen);
		return -1;
	}
}

static void getMethodInfo(ngx_http_request_t *r)
{
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->method is %d", r->method);
	
	// r->method_name
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->method_name is %V", &(r->method_name));
	
	// r->request_start到r->method_end
	u_char methodName2[r->method_end-r->request_start+1+1];
	ngx_memzero(methodName2, sizeof(methodName2)/sizeof(methodName2[0]));
	ngx_cpystrn(methodName2, r->request_start, r->method_end-r->request_start+1);
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->method_name(request_start to method_end) is %s", methodName2);
	
}

// static void getAbsoluteFileName(ngx_http_request_t *r, ngx_str_t* fileNameStr, u_char filename[])
// {	
	// ngx_memzero(filename, sizeof(filename)/sizeof(filename[0]));
	// u_char *downloadPath=(u_char*)"/root/download/";
	// ngx_int_t downloadPathLen=ngx_strlen(downloadPath)/sizeof(u_char);
	// ngx_cpystrn(filename, downloadPath, downloadPathLen+1);
	// ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "downloadPathLen is %d, filename is %s", downloadPathLen, filename);
	// ngx_cpystrn(filename+downloadPathLen, fileNameStr->data, fileNameStr->len+1); // 长度加一
	// ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "filename is %s", filename);
// }

static void getAbsoluteFileName(ngx_http_request_t *r, ngx_str_t* fileNameStr, u_char filename[])
{	
	ngx_memzero(filename, sizeof(filename)/sizeof(filename[0]));
	
	ngx_str_t downloadPath=ngx_string("/root/download/");
	ngx_cpystrn(filename, downloadPath.data, downloadPath.len+1);
	
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "downloadPathLen is %d, filename is %s", downloadPath.len, filename);
	ngx_cpystrn(filename+downloadPath.len, fileNameStr->data, fileNameStr->len+1); // 长度加一
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "filename is %s", filename);
}

static ngx_int_t ngx_http_mydownloadfile_handler(ngx_http_request_t *r)
{	
	getMethodInfo(r);
	
	if(!(r->method  &  (NGX_HTTP_GET | NGX_HTTP_HEAD)))
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "method NGX_HTTP_NOT_ALLOWED");
		return NGX_HTTP_NOT_ALLOWED;
	}
	
	// 获取uri和参数
	ngx_str_t fileNameStr;
	if(getRequestInfo(r, &fileNameStr)==-1)
	{
		return NGX_HTTP_NOT_FOUND;
	}
	
	// 获取http头部
	get_http_headers(r);
	
	// 丢弃包体
	ngx_int_t rc=ngx_http_discard_request_body(r);
	if(rc != NGX_OK)
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ngx_http_discard_request_body failed");
		return rc;
	}
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "ngx_http_discard_request_body ok");
	// TODO 读取和打印包体 11.8异步接收包体
	
	
	// ngx_str_t type = ngx_string("text/plain");
	// r->headers_out.status=NGX_HTTP_OK;
	
	// rc=ngx_http_send_header(r); // 发送http响应头
	// if(rc==NGX_ERROR  ||  rc>NGX_OK  ||  r->header_only)
	// {
		// ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ngx_http_send_header failed or header_only");
		// return rc;
	// }
	// ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "ngx_http_send_header ok");
	
	
	// 发送特定磁盘文件
	// 根据请求参数，发送指定文件, URI类似 /download/.../.../1.ts
	ngx_buf_t *b;
	b=ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	if(b==NULL)
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ngx_pcalloc failed");
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	
	//u_char* filename=(u_char*)"/root/test.txt";
	u_char filename[MAX_FILENAME_LEN];
	getAbsoluteFileName(r, &fileNameStr, filename);
	
	b->in_file=1;
	b->file=ngx_pcalloc(r->pool, sizeof(ngx_file_t));
	if(b->file == NULL)
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ngx_pcalloc failed");
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	b->file->fd=ngx_open_file(filename, NGX_FILE_RDONLY|NGX_FILE_NONBLOCK, NGX_FILE_OPEN, 0);
	b->file->log=r->connection->log;
	b->file->name.data=filename;
	//b->file->name.len=sizeof(filename)-1;
	b->file->name.len=strlen((const char*)filename);
	if(b->file->fd<=0)
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ngx_open_file failed");
		return NGX_HTTP_NOT_FOUND;
	}
	r->allow_ranges = 1; // 支持断点续传
	if(ngx_file_info(filename, &b->file->info)==NGX_FILE_ERROR)
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ngx_file_info failed");
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	b->file_pos=0;
	b->file_last=b->file->info.st_size;
	b->last_buf=1;
	
	ngx_pool_cleanup_t* cln=ngx_pool_cleanup_add(r->pool, sizeof(ngx_pool_cleanup_t));
	if(cln==NULL)
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ngx_pool_cleanup_add failed");
		return NGX_ERROR;
	}
	cln->handler=ngx_pool_cleanup_file;
	ngx_pool_cleanup_file_t *clnf=cln->data;
	clnf->fd=b->file->fd;
	clnf->name=b->file->name.data;
	clnf->log=r->pool->log;
	
	
	ngx_str_t type = ngx_string("text/plain");
	r->headers_out.status=NGX_HTTP_OK;
	r->headers_out.content_length_n = b->file->info.st_size;
	r->headers_out.content_type = type;
	// 发送http响应头
	rc=ngx_http_send_header(r);
	if(rc==NGX_ERROR  ||  rc>NGX_OK  ||  r->header_only)
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ngx_http_send_header failed");
		return rc;
	}

	ngx_chain_t out;
	out.buf=b;
	out.next=NULL;
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "to ngx_http_output_filter");
	return ngx_http_output_filter(r, &out);
}

static void get_http_headers(ngx_http_request_t *r)
{
	ngx_list_part_t *part=&r->headers_in.headers.part;
	ngx_table_elt_t *header=part->elts;
	
	size_t i;
	for(i=0; ; i++)
	{
		if(i>=part->nelts)
		{
			if(part->next==NULL)
				break;
			
			part=part->next;
			header=part->elts;
			i=0;
		}
		if(header[i].hash==0)
			continue;
		ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "%V : %V", &header[i].key, &header[i].value);
	}
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "exit get_http_headers");
}
