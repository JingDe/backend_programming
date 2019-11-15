

void* zmalloc(size_t size)
{
	void* ptr=malloc(size+PREFIX_SIZE);
	
	if(!ptr)
		zmalloc_oom_handler(size); // zmalloc_default_oom， 打印内存分配失败日志
#ifdef HAVE_MALLOC_SIZE
	update_zmalloc_stat_alloc(zmalloc_size(ptr)); // 已经对齐
	return ptr;
#else
	*((size_t*)ptr)=size; // 在分配的内存前PREFIX_SIZE字节存储此次分配内存大小size
	update_zmalloc_stat_alloc(size+PREFIX_SIZE); // 未对齐
	return (char*)ptr+PREFIX_SIZE; // 转换成char*，移动的是 PREFIX_SIZE字节偏移
#endif
}

// 对于提供分配内存大小的系统，获得malloc分配内存返回的ptr，实际底层分配的总大小：
// 包括ptr之前存储大小的字节，包括ptr返回的内存之后 用来对齐的空闲内存
#ifdef HAVE_MALLOC_SIZE
size_t zmalloc_size(void*  ptr)
{
	void* realptr=(char*)ptr-PREFIX_SIZE;
	size_t size=*((size_t*)realptr); // 分配内存大小存储在malloc函数的指针之前的内存中
	// size不包含存储大小的PREFIX_SIZE
	if(size  &  (sizeof(long)-1))
		size += sizeof(long) - (sizse & (sizeof(long)-1)); /*底层实际分配的内存是对齐的，在size/ptr之后有对齐字节*/
	return size+PREFIX_SIZE;
}
#endif

// 统计实际使用的内存大小
#define update_zmalloc_stat_alloc(__n) do{\
	size_t _n = (__n); \
	if(_n  &  (sizeof(long)-1)) \
		_n += sizeof(long) - (_n  &  (sizeof(long)-1)); \
	atomicIncr(used_memory, _n)
}while(0)
	

void zfree(void* ptr)
{
#ifndef HAVE_MALLOC_SIZE
	void* realptr;
	size_t oldsize;
#endif

	if(ptr==NULL)
		return ;
	
#ifdef HAVE_MALLOC_SIZE
	update_zmalloc_stat_free(zmalloc_size(ptr));
	free(ptr);
#else
	realptr = (char*)ptr-PREFIX_SIZE;
	oldsize = *((size_t*)realptr);
	update_zmalloc_stat_free(oldsize+PREFIX_SIZE);
	free(realptr);
#endif
}

#define update_zmalloc_stat_free(__n) do{\
	size_t _n=(__n); \
	if(_n  &  (sizeof(long)-1))\
		_n += sizeof(long)-(_n &  (sizeof(long)-1)); \
	atomicDecr(used_memory, __n); \
}while(0)