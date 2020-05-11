/*
memcached的slab内存池实现：
	一个slabclass_t表示一个slab分配器 ，能够分配特定大小的item。
	一个slabclass_t包含多个slab页
	每次分配item_max_size大小的一块内存/一个slab内存页，然后将此slab划分为多个item分配出去。
		
slabclass_t数组，包含不同大小的slab分配器。满足分配不同大小item。
	
*/

typedef struct{
	unsigned int size; //item的大小，不同大小的slabclass指的就是size不同
	// item的大小 = item结构体的大小+用户数据的大小
	// 最小的item的大小 = sizeof(item)+settings.chunk_size
	unsigned int perslab; // 每个slab包含多少个item
	// size * perslab 表示slabclass_t的每个slab页的大小
	// 不同的slabclass 的 size*perslab 都相同

	// 空闲、可分配的item
	void *slots; // 空闲item指针链表。第一个空闲item指针。
	unsigned int sl_curr; // 链表中的空闲item个数

	// slabclass已分配的slab页
	unsigned int slabs; // 此slab分配器已分配的slab的个数。即slab_list已经使用的个数。
	void **slab_list; // slab指针数组，每一个指针指向一个slab内存页
	unsigned int list_size; // slab_list数组的大小。即slab_list总个数。
	
	unsigned int killing;
	size_t requested; // 已分配的字节数
}slabclass_t;

static slabclass_t slabclass[MAX_NUMBER_OF_SLAB_CLASSES]; // 201
static int power_largest; // slabclass数组的实际大小

/*
初始化slabclass数组。
每个slabclass_t表示不同大小的slab分配器。
factor是slab大小的扩容因子。

先初始化slabclass数组数据结构；后进行内存预分配


*/
void slabs_init(const size_t limit, const double factor, const bool prealloc, const uint32_t *slab_sizes)
{
	int i=POWER_SMALLEST-1; // POWER_SMALLEST==1, 0
	unsigned int size=sizeof(item)+settings.chunk_size; 
	/* 最小的slab分配器分配的最小的item的大小。包含item结构体大小和用户数据大小。 */
	
	mem_limit=limit; 
	
	if(prealloc)
	{
		mem_base=malloc(mem_limit);
		if(mem_base != NULL)
		{
			mem_current=mem_base;
			mem_avail=mem_limit;
		}
		else
		{
			fprintf(stderr, "");
		}
	}
	
	memset(slabclass, 0, sizeof(slabclass));
	
	// item_size_max 是最大的item大小，同时是slab的大小
	while(++i < POWER_SMALLEST  &&  size<=settings.item_size_max/factor)
	{
		// 将size向 CHUNK_ALIGN_BYTES往上对齐
		if(size % CHUNK_ALIGN_BYTES)
			size += CHUNK_ALIGN_BYTES-(size%CHUNK_ALIGN_BYTES);
		
		slabclass[i].size=size; // 设置当前slab分配器分配的item的大小
		slabclass[i].preslab=settings.item_size_max/slabclass[i].size;
		size *= factor;		
	}
	
	// 最后一个slab分配器，分配的item大小是item_size_max，每个slab分配一个item。
	power_largest=i;
	slabclass[power_largest].size=setting.item_size_max;
	slabclass[power_largest].perslab=1;
	
	{
		char *t_initial_malloc=getenv("T_MEND_INITIAL_MALLOC");
		if(t_initial_malloc)
			mem_malloced=(size_t)atol(t_initial_malloc);
	}
	
	if(prealloc)
		slabs_preallocate(power_largest);
}

/*
对slabclass数组每个元素，调用do_slabs_newslab，进行预分配内存
*/
static void slabs_preallocate(const unsigned int maxslabs)
{
	int i;
	unsigned int prealloc=0;
	
	for(i=POWER_SMALLEST; i<=POWER_LARGEST; i++) // for(i=1; i<=1+maxslabs-1; i++)
	{
		if(++prealloc > maxslabs)
			return;
		if(do_slabs_newslab(i) == 0)
		{
			exit(1);
		}
	}
}

/*
为第id个slabclass预分配一个slab内存页，并划分为item。
grow_slab_list双倍增长slab_list数组，但 do_slabs_newslab 只分配一个slab内存页，插入到slab_list数组中。
*/
static int do_slabs_newslab(const unsigned int id)
{
	slabclass_t *p=&slabclass[id];
	// len 要么是 item_size_max，要么是 一个slab的大小
	int len=settings.slab_reassign ? settings.item_size_max : p->size*perslab;
	char *ptr;
	
	if((mem_limit &&  mem_alloced+len>mem_limit  &&  p->slabs>0)  || // 预分配内存用光
		(grow_slab_list(id)==0)  || // ?? 增长slab_list
		((ptr = memory_allocate((size_t)len)) == 0)) // 分配len字节内存/一个内存页
	{
		return 0;
	}
	
	memset(ptr, 0, (size_t)len);
	split_slab_page_into_freelist(ptr, id); // 将这块内存划分成一个个item
	
	p->slab_list[p->slabs++]=ptr;
	mem_alloced+=len;
	
	return 1;
}

static int grow_slab_list(const unsigned int id)
{
	slabclass_t *p=&slabclass[id];
	if(p->slabs == p->list_size) // ??
	{
		size_t new_size=(p->list_size!=0) ? p->list_size*2 : 16;
		void* new_list=realloc(p->slab_list, new_size*sizeof(void*));
		if(new_list==0)
			return 0;
		p->list_size = new_size;
		p->slab_list = new_list;
	}
	return 1;
}

static void* memory_allocate(size_t size)
{
	void* ret;
	if(mem_base ==NULL)
	{
		ret=malloc(size);
	}
	else
	{
		ret=mem_current;
		
		if(size>mem_size) // 检查可用内存大小
		{
			return NULL;
		}
		
		if(size % CHUNK_ALIGN_BYTES) // 8
			size += size%CHUNK_ALIGN_BYTES;
		mem_current=((char*)mem_current)+size;
		if(size<mem_avail)
			mem_avail-=size;
		else // 不要求满足对齐内存大小
			mem_avail=0;
	}
	return ret;
}

/*
为什么当ptr指向的内存不足item_size_max或者p->size*p->perslab时，也ok??
*/

// 将ptr指向的slab内存页分配给第id个slabclass，划分为多个item
static void split_slab_page_into_freelist(char* ptr, const unsigned int id)
{
	slabclass_t *p=&slabclass[id];
	int x;
	for(x=0; x<p->preslab; x++)
	{
		do_slabs_free(ptr, 0, id);
		ptr += p->size;
	}
}

// 释放ptr指向的内存/item，给第id个slabclass，插入到空闲item链表中。size表示分配字节大小
static void do_slabs_free(void* ptr, const size_t size, unsigned int id)
{
	slabclass_t *p;
	item *it;
	
	assert( ((item*)ptr)->slabs_clsid == 0);
	assert(id>=POWER_SMALLEST  &&  id<=power_largest);
	
	p=&slabclass[id];
	
	it=(item*)ptr;
	it->it_flags |= ITEM_SLABBED;
	it->prev=0;
	it->next=p->slots;
	if(it->next)
		it->next->prev = it;
	p->slots=it;
	
	p->sl_curr++;
	p->requested-=size;
}


/*
item 结构体
*/
typedef struct _stritem{
	struct _stritem *next;
	struct _stritem *prev;
	...
	
	uint8_t slabs_clsid; // 属于第几个slabclass_t，0表示空闲未分配
	...
}item;



/*
向第id个slabclass内存池申请大小为size的内存：
*/
static void* do_slabs_alloc(const size_t size, unsigned int id)
{
	slabclass_t *p;
	void *ret=NULL;
	item *it=NULL;
	
	if(id<POWER_SMALLEST  ||  id>power_largest)
		return NULL;
	
	p=&slabclass[id];
	assert(p->sl_curr==0  ||  ((item*)p->slots)->slabs_clsid==0); // slabs_clsid等于0表示空闲item
	
	/*
	没有空闲item在空闲链表中，分配一个slab内存页也失败
	*/
	if(!(p->sl_curr!=0  ||  do_slabs_newslab(id)!=0))
	{
		ret=NULL;
	}
	else if(p->sl_curr!=0)
	{
		it=(item*)p->slots;
		p->slots=it->next;
		if(it->next)
			it->next->prev=0;
		p->sl_curr--;
		ret=(void*)it;
	}
	
	if(ret)
	{
		p->requested+=size;
	}
	else
	{}

	return ret;
}

// unsigned int slabs_clsid(const size_t size)
// {
	// for(int i=POWER_SMALLEST; i<=power_largest; i++)
	// {
		// slabclass_t *slab=&slabclass[i];
		// if(slab->size>=size)
			// return i;
	// }
	// return 0;
// }
// 分配size大小内存时，需要从哪个slabclass中分配
unsigned int slabs_clsid(const size_t size)
{
	int res=POWER_SMALLEST;
	if(size==0)
		return 0;
	while(size>slabclass[res].size)
		if(res++ == power_largest)
			return 0;
	return res;
}


