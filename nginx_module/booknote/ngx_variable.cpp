
ngx_int_t ngx_http_get_variable_index(ngx_conf_t cf, ngx_str_t name);
// name时变量名，由某个nginx模块定义过。返回变量的索引值。

ngx_http_variable_value_t* ngx_http_get_indexed_variable(ngx_http_request *r, ngx_uint_t index);
// 根据索引值index，获得变量的值
// 变量的值随请求而不同


struct ngx_http_variable_s{
	ngx_str_t name;
	/*如果需要变量最初赋值时就进行变量值的设置。
	如果定义的内部变量允许在conf中以set方式重新设置其值。*/
	ngx_http_set_variable_pt set_handler;
	/*获取变量的值。*/
	ngx_http_get_variable_pt get_handler;
	/**get_handler/set_handler的参数*/
	uintptr_t data;
	/*变量特性*/
	ngx_uint_t flags;
	/*变量值在请求中的缓存数组中的索引, ???*/
	ngx_uint_t index;
};

typedef void (*ngx_http_set_variable_pt)(ngx_http_request_t r, ngx_http_variable_value_t v, uintptr_t data);
typedef ngx_int_t (*ngx_http_get_variable_pt)(ngx_http_request_t r, ngx_http_variable_value_t v, uintptr_t data);



typedef struct {
    unsigned    len:28; /*变量值是一段连续内存中的字符串，长度是len*/

    unsigned    valid:1; /*为1表示当前变量值已经解析过，数据可用*/
    unsigned    no_cacheable:1; /*为1表示变量值不可以被缓存,
		当ngx_http_variable_t的flags成员设置了NGX_HTTP_VARIABLE_NOCACHEABLE*/
    unsigned    not_found:1; /*为1表示当前这个变量值已经解析过，但没有解析到值*/
    unsigned    escape:1; /**/

    u_char     *data; /*指向变量值所在内存的起始地址*/
} ngx_variable_value_t; // ngx_http_variable_value_t

typedef struct{
	
	/*存储变量名的散列表*/
	ngx_hash_t variables_hash;
	/*存储索引过的变量的数组，使用变量的模块在启动时获得索引号。
		如果变量值没有被缓存，通过索引号在variables数组中找到变量的定义*/
	ngx_array_t variables;
	
	/*用于构造variables_hash散列表的初始结构体*/
	ngx_hash_keys_arrays_t *variables_keys;
	
} ngx_http_core_main_conf_t;

struct ngx_http_request_s{
	
	// variables数组存储所有序列化了的变量值，数组下标就是索引号
	ngx_http_variable_value_t *variables;
};



