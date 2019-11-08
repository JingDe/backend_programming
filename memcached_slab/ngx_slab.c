

/*
slab内存池，管理结构体
*/
typedef struct{
	ngx_shmtx_sh_t lock; // ？？
	
	size_t min_size; // 最小的内存块长度
	size_t min_shift; // min_size对应的位偏移
	
	ngx_slab_page_t *pages; // 所有页对应的ngx_slab_page_t描述结构体的数组的首地址
	ngx_slab_page_t  free; // 空闲页链表的头节点，不表示一个可用的页面
	
	u_char *start; // 第一个页面的首地址
	u_char *end; // 共享内存的尾部
	
	ngx_shmtx_t mutex; // ?? 互斥锁
	
	u_char *log_ctx; // 记录日志的描述符字符串
	u_char zero; // \0, 当log_ctx未赋值，指向zero
	
	void *data; // 由使用slab的模块自由使用
	void *addr; // 指向所属的ngx_shm_zone_t里的ngx_shm_t成员的addr成员 ？？
};

/*
slab页结构体：
*/
typedef struct ngx_slab_page_s ngx_slab_page_t;

struct ngx_slab_page_s{
	uintptr_t slab; // ??
	ngx_slab_page_t *next; // 双向链表中的下一页
	uintptr_t prev; // ？？,双向链表中的上一页
};






/*
内存分配方法：分配超大页面
*/
#define NGX_SLAB_PAGE_MASK 3 


#if (NGX_PTR_SIZE==4)
#define NGX_SLAB_PAGE_START		0x80000000
#define NGX_SLAB_PAGE_BUSY  	0xffffffff
#define NGX_SLAB_BUSY 		 	0xffffffff
#define NGX_SLAB_SHIFT_MASK 	0x0000000f
#define NGX_SLAB_MAP_SHIFT		16
#define NGX_SLAB_MAP_MASK		0xffff0000

#else // NGX_PTR_SIZE==4
#define NGX_SLAB_PAGE_START 	0x8000000000000000
#define NGX_SLAB_PAGE_BUSY		0xffffffffffffffff
#define NGX_SLAB_BUSY			0xffffffffffffffff
#define NGX_SLAB_SHIFT_MASK		0X000000000000000f
#define NGX_SLAB_MAP_SHIFT		32
#define NGX_SLAB_MAP_MASK		0xffffffff00000000

#endif

static ngx_slab_page_t* ngx_slab_alloc_pages(ngx_slab_pool_t* pool, ngx_uint_t pages)
{
	ngx_slab_page_t *page, *p;
	
	for(page=pool->free.next; page!=&pool->free; page=page->next)
	{
		if(page->slab >= pages)
		{
			if(page->slab>pages)
			{
				// 取出前pages个页面
				page[pages].slab=page->slab-pages;
				page[pages].next=page->next;
				page[pages].prev=page->prev;
				
				p=(ngx_slab_page_t*)page->prev;
				p->next=&page[pages];
				page->next->prev=(uintptr_t)&pages[pages];
			}
			else
			{
				p=(ngx_slab_page_t*)page->prev;
				p->next=page->next;
				page->next->prev=page->prev;// page->next->prev=page->prev;
			}
			
			/* 超大内存分配时，第一个页面的slab成员的高三位设置为100，低位时连续分配的页面总数;
			prev成员设为NGX_SLAB_PAGE表示是以整页来使用*/
			page->slab=pages | NGX_SLAB_PAGE_START;
			page->next=NULL;
			page->prev=NGX_SLAB_PAGE; // 0
			
			if(--pages==0) // if(pages==1)
				return page;
			
			/* 超大内存分配时，除第一个页面往后的页面，slab成员设置为BUSY*/
			for(p=page+1; pages; pages--)
			{
				p->slab=NGX_SLAB_PAGE_BUSY;
				p->next=NULL;
				p->prev=NGX_SLAB_PAGE; // 0
				p++;
			}
			
			return page;
		}
	}
	
	ngx_slab_error();
	
	return NULL;
}



/*
完整的内存分配方法：
*/

void* ngx_slab_alloc_locked(ngx_slab_pool_t *pool, size_t size)
{
	size_t s;
	uintptr_t p, n, m, mask, *bitmap;
	ngx_uint_t i, slot, shift, map;
	ngx_slab_page_t *page, *prev, *slots;
	
	if(size >= ngx_slab_max_size)
	{
		page=ngx_slab_alloc_pages(pool, (size+ngx_pagesize-1)>>ngx_pagesize_shift);
		/* +ngx_pagesize-1 使得计算出的页面向上取整
			>> ngx_pagesize_shift, 快速计算除以ngx_pagesize 
			返回对应页面的描述结构体指针 */
		
		if(page)
		{
			p = (page-pool->pages) << ngx_pagesize_shift;
			/* page-pool->pages表示返回的页面是pages数组中第几个页面
				<< ngx_pagesize_shift 是分配页面相对第一个页面首地址的偏移*/
			p += (uintptr_t) pool->start; // 获得分配页面的首地址
		}
		else
		{
			p=0;
		}
		goto done;
	}
	
	if(size>pool->min_size) // 请求大小大于最小的内存块大小
	{
		shift=1;
		for(s=size-1; s>>=1; shift++) // 
		{}
		slot=shift-pool->min_shift; // 获得slots数组slot下标的半满页链表
		/*
		假设min_size==8, min_shift==3; size=16
		shift=4,
		slot=1，slots[1]存放大小为16的内存块半满页链表
		*/
	}
	else
	{
		size=pool->min_size;
		shift=pool->min_shift;
		slot=0;
	}
	
	slots=(ngx_slab_page_t*) ((u_char*)pool+sizeof(ngx_slab_page_t));
	page=slots[slot].next;
	
	/* 当对应半满页链表不为空时，遍历半满页链表, page指向第一个页描述结构体的地址 */
	if(page->next != page)
	{
		/* shift表示内存块大小的位偏移 */
		if(shift<ngx_slab_exact_shift)
		{
			do{
				p=(page-pool->pages) << ngx_pagesize_shift;
				/* page-pool->pages表示当前页在pages数组中的位置，
				<<ngx_pagesize_shift表示当前页相对start的偏移字节 */
				bitmap = (uintptr_t*) (pool->start+p); 
				/* 对小块内存，页面开始位置存储的是bitmap */
				
				map =(1<<(ngx_pagesize_shift-shift)) / (sizeof(uintptr_t)*8);
				/* (1<<(ngx_pagesize_shift-shift))等价于 页面大小/内存块大小，等于内存块个数。
					/ (sizeof(uintptr_t)*8)等于需要多少个uintptr_t存储bitmap */
				
				/* 遍历bitmap */
				for(n=0; n<map; n++)
				{
					if(bitmap[n]!=NGX_SLAB_BUSY)// 存在空闲块
					{
						for(m=1, i=0; m; m<<=1, i++)
						{
							if(bitmap[n]  &  m) // bitmap[n]从低往高第i位是1，已分配
								continue;
							bitmap[n] |= m; // bitmap[n]从低往高第i位指向内存块未被分配
							
							i=((n*sizeof(uintptr_t)*8) << shift) + (i<<shift);
							/* 等价于 i = (n*sizeof(uintptr_t)*8 + i) << shift;
								n*sizeof(uintptr_t)*8+i 表示该空闲块是半满页中的第几个内存块，
								<<shift表示对应内存地址相对半满页首地址的偏移 */
							
							/* 检查是否是当前半满页中最后一个空闲内存块 */
							if(bitmap[n] == NGX_SLAB_BUSY)
							{
								for(n=n+1; n<map; n++)
								{
									if(bitmap[m] != NGX_SLAB_BUSY)
									{
										p=(uintptr_t) bitmap+i; // bitmap同时是半满页首地址
										goto done;
									}
								}
								// 需要将半满页脱离半满页链表
								prev=(ngx_slab_page_t*) (page->prev  &  ~NGX_SLAB_PAGE_MASK);
								/*小块内存块的半满页描述结构体的prev成员，？？							
								获得真实的前一个页节点的指针 ？？ */
								prev->next=page->next;
								page->next->prev = page->prev;
								
								// 设置全满页的next和prev成员
								page->next=NULL;
								page->prev=NGX_SLAB_SMALL;
							}
							
							/*无论是完成了全满页的脱离，还是未成为全满页，返回的空闲内存块地址都不变*/
							p=(uintptr_t) bitmap+i;
							goto done; 
						}
					}
				}				
				page=page->next;
			}while(page);			
		}
		else if(shift==ngx_slab_exact_shift)
		{
			do{
				/*对中等内存，bitmap就存储在slab成员中*/
				if(page->slab != NGX_SLAB_BUSY)
				{
					for(m=1, i=0; m; m<<=1, i++)
					{
						if(page->slab & m)
							continue;
						
						/*page指向的半满页的第i个内存块是空闲的 */
						page->slab |= m;
						if(page->slab == NGX_SLAB_BUSY) // 第i个内存块是最后一个空闲内存块
						{
							prev = (ngx_slab_page_t*) (page->prev  &  ~NGX_SLAB_PAGE_MASK);
							prev->next =  page->next;
							page->next->prev = page->prev;
							
							// 设置全满的中等页的next和prev成员
							page->next=NULL;
							page->prev=NGX_SLAB_EXACT;
						}
						
						p = (page-pool->pages) << ngx_pagesize_shift;
						/* (page-pool->pages)表示page是第几个页，<< ngx_pagesize_shift表示页首地址相对于start的偏移 */
						p += i<<shift; /*i*shift表示第i个空闲内存块，相对于页首地址的偏移 */
						p += (uintptr_t) pool->start; /*p获得空闲内存块的地址*/
						goto done;
					}
				}
				page=page->next;
			}while(page);
		}
		else /*请求分配大块内存*/
		{
			n = ngx_pagesize_shift - (page->slab  &  NGX_SLAB_SHIFT_MASK); // 最低字节掩码
			/* 取page的slab成员最低一个字节，表示内存块的大小的移位数。？？ */
			n = 1<<n;
			/* 此时，n等价于 ngx_slab_max_size除以内存块大小，等于内存块的个数，即需要的bitmap位数 */
			n = ((uintptr_t) 1<<n) -1;
			/* 当bitmap每一位都为1时，对应的整数值 */
			mask = n<<NGX_SLAB_MAP_SHIFT; // slab成员位数的一半
			/* 左移最大的bitmap，到slab用来表示bitmap的对应高位字节位置 */
			
			do{
				/* 取slab中高位一半字节存储的bitmap，不都是1，存在空闲内存块 */
				if((page->slab  &  NGX_SLAB_MAP_MASK) != mask)					
				{
					for(m=(uintptr_t)1 << NGX_SLAB_MAP_SHIFT, i=0; m & mask; m<<=1, i++)
					{
						if(page->slab  &  m)
							continue;
						
						// bitmap的第i位是0
						page->slab |= m;
						// 最后一个空闲内存块
						if(page->slab  &  NGX_SLAB_MAP_MASK == mask)
						{
							prev = (ngx_slab_page_t*) (page->prev  &  ~NGX_SLAB_PAGE_MASK);
							prev->next = page->next;
							page->next->prev = page->prev;
							
							// 大块内存的全满页
							page->next = NULL;
							page->prev = NGX_SLAB_BIG;
						}
						
						p = (page-pool->pages) << ngx_pagesize_shift;
						p += i<<shift;
						p += (uintptr_t) pool->start;
						goto done;
					}
				}
				page=page->next;
			}while(page);
		}
	}
	
	// 从free链表中分配一个空闲页
	page=ngx_slab_alloc_pages(pool, 1);
	
	if(page)
	{
		if(shift < ngx_slab_exact_shift)
		{
			/*
			当从一个新页面分配一个小块内存时：		
				先置存储bitmap的1个或者多个内存块，对应的bitmap位为1，
				后置新分配的内存块，对应的bitmap位为1。？？？
			*/
			
			p = (page-pool->pages) << ngx_pagesize_shift;
			bitmap = (uintptr_t*) (pool->start+p); /* 设置小块内存的bitmap */
			
			s=1<<shift; // s等于内存块大小
			n= (1<<(ngx_pagesize_shift-shift)) / 8 / s;
			/* s等于小块内存块的大小， 
				(1<<(ngx_pagesize_shift-shift))等于内存块的个数，
				(1<<(ngx_pagesize_shift-shift)) / 8，表示需要多少个字节表示bitmap,
				n表示表示bitmap需要多少个内存块存放。
				(此时n等于2的x幂次方，n是正整数，或者小于1) */
			
			if(n==0)
				n=1; 
			/* 当小块内存块较大时，一个内存块可以放下表示所有内存块的bitmap，
				此时分配第一个内存块。 */
			bitmap[0] = (2<<n) -1; // (1<<(n+1))-1
			/* （小块内存的bitmap需要超过一个uintptr_t的位数来表示），
				bitmap[0]表示类型是uintptr_t，设置低n+1个bit为1，表示前n个内存块被分配用作bitmap。下一个内存块新分配出去。
				*/
			
				
			map = (1<<(ngx_pagesize_shift-shift)) / (sizeof(uintptr_t)*8);
			/* map表示需要多少个uintptr表示bitmap */
			for(i=1; i<map; i++)
			{
				bitmap[i]=0; /*设置未分配的内存块的bitmap为未分配状态 */
			}
			
			/*将新页面插入到对应半满页链表开头 */
			page->slab = shift;
			page->next = &slots[slot];
			page->prev = (uintptr_t)&slots[slot] | NGX_SLAB_SMALL;
			
			slots[slot].next=page;
			
			p = ((page-pool->pages) << ngx_pagesize_shift) + s*n;
			/* 前n个内存块用于bitmap，第一个可分配内存块的相对地址+s*n */
			p += (uintptr_t) pool->start;
			goto done;
		}
		else  if(shift==ngx_slab_exact_shift)
		{
			page->slab=1;
			page->next=&slots[slot];
			page->prev=(uintptr_t) &slots[slot] | NGX_SLAB_EXACT;
			
			slots[slot].next=page;
			
			p = (page-pool->pages) << ngx_pagesize_shift;
			p += (uintptr_t) pool->start;
			goto done;
		}
		else // shift > ngx_slab_exact_shift
		{
			page->slab = ((uintptr_t)1 << NGX_SLAB_MAP_SHIFT) | shift;
			page->next=&slots[slot];
			page->prev = (uintptr_t)&slots[slot] | NGX_SLAB_BIG;
			
			slots[slot].next=page;
			
			p=(page-pool->pages)<<ngx_pagesize_shift;
			p += (uintptr_t) pool->start;
			goto done;
		}
	}
	
	p=0;
	
done:
	return (void*) p;
}


void ngx_slab_free_locked(ngx_slab_pool_t *pool, void *p)
{
	size_t size;
	uintptr_t slab, m, *bitmap;
	ngx_uint_t n, type, slot, shift, map;
	ngx_slab_page_t *slots, *page;
	
	if((u_char*)p<pool->start  ||  (u_char*)p>pool->end)
	{
		ngx_slab_error();
		goto fail;
	}
	
	n = ((u_char*)p-pool->start) >> ngx_pagesize_shift;
	page = &pool->pages[n];
	slab = page->slab;
	type = page->prev  &  NGX_SLAB_PAGE_MASK; // 
	
fail:
	return;
}





