
/*
	用volatile关键字在C语言级别上模拟原子操作
	
	ngx_atomic_cmp_set方法会将old参数与原子变量lock的值做比较，
	如果它们相等，则把lock设为参数set，同时方法返回1；
	如果它们不相等，则不做任何修改，返回0。
*/
typedef long ngx_atomic_int_t;
typedef unsigned long ngx_atomic_uint_t;
typedef volatile ngx_atomic_uint_t ngx_atomic_t;//volatile关键字告诉C编译器不要做优化

static inline ngx_atomic_uint_t
ngx_atomic_cmp_set(ngx_atomic_t *lock, ngx_atomic_uint_t old, ngx_atomic_uint_t set);







static inline ngx_atomic_uint_t
ngx_atomic_cmp_set(ngx_atomic_t *lock, ngx_atomic_uint_t old, ngx_atomic_uint_t set)
{
	if(*lock==old)
	{
		*lock=set;
		return 1;
	}
	return 0;
}

static inline ngx_atomic_int_t
ngx_atomic_fetch_add(ngx_atomic_t *value, ngx_atomic_int_t add)
{
	ngx_atomic_int_t old;
	old=*value;
	*value+=add;
	return old;
}

