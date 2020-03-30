#include"sqlite_demo.h"
#include"pcache.h"

/*
默认的plugin cache模块实现: PCache1 (sqlite默认的pcache1)
*/

#define PCACHE1_LRU_ADD 1
#define PCACHE1_LRU_REMOVE 2

typedef struct PgHdr1 PgHdr1;
typedef struct PCache1 PCache1;

/* PCache1 的每一个页面 使用 PgHdr1 结构体表示*/
struct PgHdr1{
	SqlPCachePage base; // 从 PCache 模块获得
	
	unsigned int key; // key是页面编号
	unsigned char is_pinned; // 是否使用
	PgHdr1 *pnext; // 哈希表中同一个slot的冲突链的下一个节点
	PCache1 *pcache; // 被管理的 PCache1 模块
	
	PgHdr1 *plru_next; // lru链表中下一个节点
	PgHdr1 *plru_prev; 
};

// PCache1 模块
struct PCache1{
	PgHdr1 *plru_head; // LRU链表头,存储unpinned不使用、已分配的页面，不计入mx_pages和npage中
	PgHdr1 *plru_tail; // 
	
	int sz_page; // 页面大小
	int sz_extra; // 附加内容的大小
	int mx_pages; // 可以缓存在内存中的最大页面数, npage的最大值
	
	unsigned int nhash; // 哈希表的slot数量
	unsigned int npage; // 哈希表的页面总数
	
	PgHdr1 **aphash; // 哈希表
};

static void _pcache1ResizeHash(PCache1* pcache);
static void _pcache1AddRemoveLRU(PgHdr1 *page, int flags);
static void _pcache1RemoveFromHash(PgHdr1 *page, int free_flag);
static void _pcache1AddRemoveLRU(PgHdr1 *page, int flags);

// 创建 PCache1模块
static SqlPCache* _pcache1Create(int sz_page, int sz_extra, int mx_pages)
{
	PCache1* pcache=(PCache1*)malloc(sizeof(PCache1));
	if(pcache==0)
		return 0;
	memset(pcache, 0, sizeof(PCache1));
	pcache->sz_page=sz_page;
	pcache->sz_extra=sz_extra;
	pcache->mx_pages=mx_pages;
	_pcache1ResizeHash(pcache); // 创建hash数组
	
	assert(pcache->nhash);
	return (SqlPCache*)pcache;
}

// 扩展哈希表
static void _pcache1ResizeHash(PCache1* pcache)
{
	PgHdr1 **apnew;
	unsigned int nnew;
	
	nnew=pcache->nhash*2; // 扩大一倍
	if(nnew<256)
		nnew=256;
	
	apnew=(PgHdr1**)malloc(sizeof(PgHdr1*)*nnew);
	if(apnew)
	{
		int i;
		// 将旧哈希表中的所有节点迁移到apnew中
		for(i=0; i<pcache->nhash; ++i)
		{
			PgHdr1 *p;
			PgHdr1 *pnext=pcache->aphash[i];
			while((p=pnext) != 0)
			{
				unsigned int h=p->key  %  nnew;
				pnext=p->pnext;
				
				p->pnext=apnew[h];
				apnew[h]=p;
			}
		}
		
		free(pcache->aphash);
		pcache->aphash=apnew;
		pcache->nhash=nnew;
	}
}

/*
PCache1 模块，查找某个page number的页面。
PCache1 的hash表的key 是page number，
PCache1 使用的PgHdr1结构体的哈希表，包含 SqlPCachePage结构体 TODO
*/
static SqlPCachePage *_pcache1Get(SqlPCache *pcache, unsigned int key)
{
	PCache1 *pcache1=(PCache1*)pcache;
	
	// 使用最简单的mod哈希方法
	unsigned int h=key % pcache1->nhash;
	PgHdr1 *p=pcache1->aphash[h];
	while(p  &&  p->key!=key)
		p=p->pnext;
	
	if(p)
		return &(p->base); // 返回
	return 0;
}

// 根据页面编号获取页面，不存在时创建并返回
static SqlPCachePage *_pcache1Fetch(SqlPCache *pcache, unsigned int key)
{
    printf("in _pcache1Fetch\n");
	PCache1 *pcache1=(PCache1*)pcache;
	
	unsigned int h=key % pcache1->nhash;	
    printf("key %d, nhash %d, slot %d\n", key, pcache1->nhash, h);
	PgHdr1 *p=pcache1->aphash[h];
	while(p  &&  p->key!=key)
		p=p->pnext;
	
	if(p)
    {
        printf("found key %d page in aphash\n", key);
		return &(p->base); // 返回
    }

    printf("not found key %d page in aphash %d\n", key, h);
	
	if(pcache1->plru_head)
	{
        printf("lru list not empty, get the head page\n");
		p=pcache1->plru_head;
		_pcache1AddRemoveLRU(p, PCACHE1_LRU_REMOVE);
	}
	
	// 如果既没有找到页面，而且缓存页面数达到了mx_pages，返回0
	if(p==0  &&  pcache1->npage>=pcache1->mx_pages)
	{
        printf("not found, and reache the cache mx_pages, %d\n", pcache1->mx_pages);
        return 0;	
    }
	
/* 要么p!=0, 从unpinned页面链表/lru链表中获取一个不使用的页面;
 要么p==0，但是没有达到mx_pages*/
	
	// 第二种情况，lru链表中没有unpinned的页面，允许新分配一个
	if(p==0)
	{
        printf("alloc new page for key %d\n", key);
        // 页面大小 + PCache1->sz_extra对应PCache管理结构体PgHdr的大小 + PCache1的管理结构体PgHdr1的大小
		int alloc_bytes=pcache1->sz_page+pcache1->sz_extra+sizeof(PgHdr1);
		void *pnew=malloc(alloc_bytes);
		memset(pnew, 0, alloc_bytes);
		p=(PgHdr1*)(pnew+pcache1->sz_page+pcache1->sz_extra);
        // PgHdr1 的base成员，记录页面内容和PgHdr结构体的首地址
		p->base.content=pnew;
		p->base.extra=pnew+pcache1->sz_page;

        PgHdr* pghdr=(PgHdr*)p->base.extra;
        pghdr->pdata=p->base.content;
	}
	
	// 如果是第一种情况，p是从lru链表头取出的页面
	
	p->key=key;
	p->pnext=pcache1->aphash[h]; // 新页面插入到缓存页面的哈希表中
	p->is_pinned=1;
	p->pcache=pcache1;
	
	pcache1->aphash[h]=p;
	++pcache1->npage; // npage记录的是aphash哈希表中页面数
	
	return &(p->base);
}

static void _pcache1Unpin(SqlPCachePage *page)
{
	PgHdr1 *p=(PgHdr1*)page;
	assert(p);
	_pcache1RemoveFromHash(p, 0); // 将页面从哈希表中删除
	_pcache1AddRemoveLRU(p, PCACHE1_LRU_ADD); // 将页面添加到LRU链表中
}

// 从哈希表中删除一个页面，free_flag表示是否释放 TODO
static void _pcache1RemoveFromHash(PgHdr1 *page, int free_flag)
{
	unsigned int h;
	PCache1 *pcache=page->pcache; // 页面所在的 PCache1
	PgHdr1 **pp;
	
	h=page->key % pcache->nhash;
	for(pp=&pcache->aphash[h]; (*pp)!=page  &&  (*pp); pp=&(*pp)->pnext)
	{}

	assert(*pp==page);
	*pp=(*pp)->pnext; // 删除一个节点，直接等于修改指向对应节点的指针的值
	
	--pcache->npage;
}


// 根据flags，从lru链表中添加或者删除一个页面
static void _pcache1AddRemoveLRU(PgHdr1 *page, int flags)
{
	PCache1 *pcache1=page->pcache;
	assert(pcache1);
	
	if(flags  &  PCACHE1_LRU_ADD)
	{
		page->plru_next=pcache1->plru_head;
		page->plru_prev=0;
		pcache1->plru_head=page;
		if(pcache1->plru_tail==0)
			pcache1->plru_tail=page;
	}
	
	if(flags  &  PCACHE1_LRU_REMOVE)
	{
		if(page->plru_prev)
			page->plru_prev->plru_next=page->plru_next;
		if(page->plru_next)
			page->plru_next->plru_prev=page->plru_prev;
		if(pcache1->plru_head==page)
			pcache1->plru_head=page->plru_next;
		if(pcache1->plru_tail==page)
			pcache1->plru_tail=page->plru_prev;
	}
	
}

// 设置全局的plugin cache模块的结构的各个方法
void pcache1GlobalRegister(SqlPCacheMethods *methods)
{
	methods->xCreate=_pcache1Create; // PCache1 创建方法
	methods->xGet=_pcache1Get; // PCache1 查找页面方法，不存在返回0
	methods->xFetch=_pcache1Fetch; // 获取页面方法，不存在创建并返回 
	methods->xUnpin=_pcache1Unpin; // unpin页面，页面从使用到不使用
}
