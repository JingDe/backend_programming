
/*
默认的plugin cache模块实现: PCache1 (sqlite默认的pcache1)
*/



/* PCache1 的每一个页面 使用 PgHdr1 结构体表示*/
typedef struct PgHdr1 PgHdr1;
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
	PgHdr1 *plru_head; // LRU链表头
	PgHdr1 *plru_tail; // 
	
	int sz_page; // 页面大小
	int sz_extra; // 附加内容的大小
	int max_pages; // 可以缓存在内存中的最大页面数
	
	unsigned int nhash; // 哈希表的slot数量
	unsigned int npage; // 哈希表的页面总数
	
	PgHdr1 **aphash; // 哈希表
};


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
	PCach1 *pcache=page->pcache; // 页面所在的 PCache1
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
	PCach1 *pcache1=page->pcache;
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
	methods->xGet=_pcache1Get; // PCache1 查找页面方法
	methods->xUnpin=_pcache1Unpin; // unpin页面，页面从使用到不使用
}