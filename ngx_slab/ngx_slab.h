
#ifndef NGX_SLAB_H
#define NGX_SLAB_H

#include<cstdint>

#include<unistd.h>// getpagesize()

unsigned int  ngx_pagesize;//页面大小
unsigned int  ngx_pagesize_shift;//页大小对应的位移变量


// 描述每一种内存块的分配情况的结构，对应每个slots数组元素
typedef struct{
	unsigned int total;
	unsigned int used;
	
	unsigned int reqs;
	unsigned int fails;
}ngx_slab_stat_t;



// 描述slab共享内存池的结构
typedef struct{	
	//信号量
	
	size_t min_size;//设定的最小内存块长度
	size_t min_shift;//min_size对应的位偏移
	
	ngx_slab_page_t *pages;//每一页对应一个ngx_slab_page_t页描述结构体，所有的存放在连续的内存中构成数组，pages是首地址
	ngx_slab_page_t *last;
	ngx_slab_page_t free;//所有的空闲页组成一个链表挂在free成员上
						//free结构体不表示一个实际页面
	
	ngx_slab_stat_t *stats;
	unsigned int pfree;
	
	unsigned char *start;//所有的实际页面全部连续地放在一起，第1页的首地址是start
	unsigned char *end;//指向这段共享内存的尾部
	
	Mutex mutex;//互斥锁
	
	/*slab操作失败时会记录日志，为区别是哪个slab共享内存出错，
	可以在slab中分配一段内存存放描述的字符串，
	然后再用log_ctx指向这个字符串*/
	unsigned char *log_ctx;
	unsigned char zero;//\0，当log_ctx没有赋值时，将直接指向zero
	
	unsigned log_nomem:1;
	
	void *data;//由各个使用slab的模块自由使用，slab管理内存时不会用到
	void *addr;//指向所属的ngx_shm_zone_t里的ngx_shm_t成员的addr成员
}ngx_slab_pool_t;
/* 一个slab共享内存池的示意图：
	*	ngx_slab_pool_t结构体
	*	slots数组：ngx_slab_page_t数组，长度是内存块种类的个数。
				每一个元素链接起一条pages数组元素（具有相同内存块大小）。
	*	stat数组：长度是内存块种类的个数
	*	pages数组：ngx_slab_page_t结构体数组，长度是页面个数。ngx_slab_pool_t的pages成员指向这里
	*	对齐内存
	*	所有真实的页面：包括空闲页，半满页，全满页。ngx_slab_pool_t的start成员指向这里
	*	......页面......
	*	对齐内存

*

*/



// 描述一个页面的结构
// 全满页不在任何链表中，它对应的ngx_slab_page_t中的next和prev成员没有任何链表功能。
// 所有的空闲页构成1个双向链表，ngx_slab_pool_t中的free指向这个链表
	// 首页面的slab成员大于1时则表示其后有相邻的空闲页面
typedef struct ngx_slab_page_s ngx_slab_page_t;

struct ngx_slab_page_s{
	uintptr_t slab;
	ngx_slab_page_t *next;//指向双向链表中的下一页
	uintptr_t prev;//同时用于指向双向链表中的上一页
};


void ngx_slab_sizes_init(void);
void ngx_slab_init(ngx_slab_pool_t *pool);
void *ngx_slab_alloc(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_alloc_locked(ngx_slab_pool_t *pool, size_t size);

void *ngx_slab_calloc(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_calloc_locked(ngx_slab_pool_t *pool, size_t size);
void ngx_slab_free(ngx_slab_pool_t *pool, void *p);
void ngx_slab_free_locked(ngx_slab_pool_t *pool, void *p);

#endif