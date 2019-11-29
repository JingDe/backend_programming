#include<ngx_http.h>
#include<nginx.h>


static char* ngx_http_myallownew(ngx_conf_t* cf, ngx_command_t* cmd, void* conf); // 解析配置项
static ngx_int_t ngx_http_myvariablenew_handler(ngx_http_request_t *r); // access阶段回调方法
static ngx_int_t ngx_http_myvariablenew_init(ngx_conf_t* cf); // 配置项解析完成回调方法，模块初始化方法
static void* ngx_http_myvariablenew_create_loc_conf(ngx_conf_t* cf); // 创建配置项结构体

static ngx_str_t new_variable_is_chrome = ngx_string("is_chrome");

static ngx_int_t ngx_http_myvariablenew_add_variable(ngx_conf_t *cf); // preconfiguration方法
static ngx_int_t ngx_http_ischrome_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data); // 变量解析方法

// 模块接口
static ngx_http_module_t ngx_http_myvariablenew_module_ctx={
	ngx_http_myvariablenew_add_variable, // 不需要在解析配置项之前添加新变量
	ngx_http_myvariablenew_init, // 解析配置完成后回调
	NULL, // myallow配置不能存在于http和server配置下
	NULL,
	NULL, 
	NULL,
	ngx_http_myvariablenew_create_loc_conf, // 生成存放location下myallow配置的结构体
	NULL
};

static ngx_int_t ngx_http_myvariablenew_add_variable(ngx_conf_t *cf)
{
	ngx_http_variable_t *v;
	v=ngx_http_add_variable(cf, &new_variable_is_chrome, NGX_HTTP_VAR_CHANGEABLE);
	if(v==NULL)
		return NGX_ERROR;
	v->get_handler =ngx_http_ischrome_variable;
	v->data=0;
	return NGX_OK;
}

static ngx_int_t ngx_http_ischrome_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
	if(r->headers_in.chrome)
	{
		*v=ngx_http_variable_true_value;
		return NGX_OK;
	}
	*v = ngx_http_variable_null_value; 
	return NGX_OK;
}

// location配置项结构体
typedef struct{
	int variablenew_index; // 变量的索引值
	ngx_str_t variablenew; // myallow配置后第一个参数，表示待处理变量名
	ngx_str_t equalvalue; // 变量名必须为equalvalue才能放行请求
} ngx_myallow_loc_conf_t;

static void* ngx_http_myvariablenew_create_loc_conf(ngx_conf_t* cf)
{
	ngx_myallow_loc_conf_t *conf;
	conf = ngx_pcalloc(cf->pool, sizeof(ngx_myallow_loc_conf_t));
	if(conf==NULL)
		return NULL;
	conf->variablenew_index=-1;
	return conf;
}

// 
static ngx_int_t ngx_http_myvariablenew_init(ngx_conf_t* cf)
{
	ngx_http_handler_pt *h;
	ngx_http_core_main_conf_t *cmcf;
	
	// 取出全局唯一的核心结构体
	cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
	h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
	if(h==NULL)
		return NGX_ERROR;
	
	*h = ngx_http_myvariablenew_handler; // 处理请求的方法
	return NGX_OK;
}


// myvariablenew模块关心的配置项
static ngx_command_t ngx_http_myvariablenew_commands[]={
	{
		ngx_string("myallownew"),
		NGX_HTTP_LOC_CONF | NGX_CONF_TAKE2,
		ngx_http_myallownew,
		NGX_HTTP_LOC_CONF_OFFSET, // ??
		0,
		NULL
	},
	ngx_null_command	
};

ngx_module_t ngx_http_myvariablenew_module={
	NGX_MODULE_V1,
	
	&ngx_http_myvariablenew_module_ctx, // 模块接口
	ngx_http_myvariablenew_commands,
	NGX_HTTP_MODULE, // 模块类型
	
	NULL,
	NULL, /* nginx启动时，worker启动前，调用各个模块的初始化回调方法，myvariablenew模块不需要*/
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	
	NGX_MODULE_V1_PADDING
};


// 配置项myallow解析方法
// myallow $remote_addr 192.168.55.200
static char* ngx_http_myallownew(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
	ngx_log_debug(NGX_LOG_DEBUG, cf->log, 0, "in ngx_http_myallownew");
	ngx_str_t *value;
	ngx_myallow_loc_conf_t *macf =conf; // ???
	value =cf->args->elts; // value指向args数组首地址
	// cf->args有三个成员，myallow加上两个参数
	if(cf->args->nelts !=3)
	{
		return NGX_CONF_ERROR;
	}
	// 第一个参数必须是$开头
	if(value[1].data[0] == '$')
	{
		value[1].len--;
		value[1].data++;
		macf->variablenew_index = ngx_http_get_variable_index(cf, &value[1]);
		if(macf->variablenew_index == NGX_ERROR)
		{
			ngx_str_t tmp_var={value[1].len, value[1].data};
			ngx_log_error(NGX_LOG_ERR, cf->log, 0, "ngx_http_get_variable_index failed, %V", &tmp_var);
			return NGX_CONF_ERROR;
		}
		macf->variablenew = value[1];
	}
	else
	{
		return NGX_CONF_ERROR;
	}
	macf->equalvalue=value[2];
	return NGX_CONF_OK;
}

// 在access阶段被按照模块顺序调用
static ngx_int_t ngx_http_myvariablenew_handler(ngx_http_request_t *r)
{
	ngx_myallow_loc_conf_t *conf;
	ngx_http_variable_value_t *vv;
	
	conf = ngx_http_get_module_loc_conf(r, ngx_http_myvariablenew_module);
	if(conf==NULL)
		return NGX_ERROR;
	
	if(conf->variablenew_index ==-1)
		return NGX_DECLINED; // 放行
	
	vv = ngx_http_get_indexed_variable(r, conf->variablenew_index);
	if(vv ==NULL  ||  vv->not_found)
		return NGX_HTTP_FORBIDDEN;
	
	ngx_str_t tmp_data={vv->len, vv->data};
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "conf->equalvalue is %V, vv->data is %V", &conf->equalvalue, &tmp_data);
	
	if(vv->len == conf->equalvalue.len  &&  0==ngx_strncmp(conf->equalvalue.data, vv->data, vv->len))
	{
		return NGX_DECLINED;
	}
		
	return NGX_HTTP_FORBIDDEN;
}
