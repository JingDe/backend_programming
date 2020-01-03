#include"sqlite_demo.h"
#include"sqliteint.h"
#include"pcache.h"

#define PCACHE_FINISH_CONVERT_NEED_INIT 1
#define PCACHE_FINISH_CONVERT_DONT_INIT 2

struct PCache{
	PgHdr *pdirty_head, *pdirty_tail; // 脏页链表头指针和尾指针
	int nref; // 这个cache模块对这个数据库文件的页面的引用总数
	int sz_page; // 页面大小
	int sz_extra; // ?? 每个cache中的额外空间？？
	SqlPCache *pcache; // pluginable的cache模块 TODO
};

static int _pcacheSetPageSize(PCache *pcache, int sz_page);
static PgHdr*_pcacheFinishConvert(
    SqlPCachePage *p, // PCache1 模块获得的 页面
    PCache *pcache, // 页面所在的cache
    Pgno pgno, // page number
    int flags);
static void _pcacheFinishConvertInit(SqlPCachePage *p, PCache *pcache, Pgno pgno);


int pcacheSize()
{
	return sizeof(PCache);
}

// 打开一个cache模块
int pcacheOpen(int sz_page, // 每个页面的大小
		int sz_extra, // 每个页面的额外空间
		PCache* pcache) // 预分配的 PCache
{
	memset(pcache, 0, sizeof(PCache));
	//pcache->sz_page=1; // ?? sz_page ???
	pcache->sz_page=sz_page;
	pcache->sz_extra=sz_extra;
	return _pcacheSetPageSize(pcache, sz_page);
}

// 创建plugin pcache模块，设置页面大小
static int _pcacheSetPageSize(PCache *pcache, int sz_page)
{
	assert(pcache->nref==0  &&  pcache->pdirty_head==0);
	if(pcache->sz_page)
	{
		SqlPCache *pnew=0;
		pnew=global_config.sql_pcache_methods.xCreate(sz_page, sizeof(PgHdr), MAX_PAGES_IN_CACHE); // 页面大小，额外数据大小，最多页面数
		if(pnew==0)
			return SQL_ERROR;
		
		pcache->pcache=pnew;
		pcache->sz_page=sz_page;
	}
    return SQL_OK;
}

// 根据页面编号，从缓存中查找页面，如果不存在缓存中，返回0
PgHdr* pcacheGet(PCache *pcache, Pgno pgno)
{
	SqlPCachePage *ppage=global_config.sql_pcache_methods.xGet(pcache->pcache, pgno);
	
	if(ppage==0)
		return 0;
	// 将 SqlPCachePage 类型转换成 PgHdr 类型
	return _pcacheFinishConvert(ppage, pcache, pgno, PCACHE_FINISH_CONVERT_DONT_INIT);
}

/*
通过页面编号获得页面。如果不在cache中，就创建一个页面返回
*/
PgHdr *pcacheFetch(PCache* pcache, Pgno pgno)
{
	SqlPCachePage *ppage=global_config.sql_pcache_methods.xFetch(pcache->pcache, pgno);
	
	if(ppage==0)
		return 0;
	
	return _pcacheFinishConvert(ppage, pcache, pgno, PCACHE_FINISH_CONVERT_NEED_INIT);
}

// 将 SqlPCachePage 类型转换成 PgHdr 类型
static PgHdr* _pcacheFinishConvert(
	SqlPCachePage *p, // PCache1 模块获得的 页面
	PCache *pcache, // 页面所在的cache
	Pgno pgno, // page number
	int flags) 
{
	assert(p!=0);
	PgHdr *pgd=(PgHdr*)p->extra; // 
	
	if(flags  &  PCACHE_FINISH_CONVERT_NEED_INIT)
	{
		_pcacheFinishConvertInit(p, pcache, pgno);
	}
	++pgd->nref; // 怎加页面的引用数
	return pgd;
}

static void _pcacheFinishConvertInit(SqlPCachePage *p, PCache *pcache, Pgno pgno)
{
	PgHdr *ret;
	ret=(PgHdr*)p->extra;
	
	ret->ppage=p;
	ret->pdata=p->content;
	/* ret->pextra=p->extra; 
	memset(ret->pextra,, 0, pcache->sz_extra); */
	// TODO ??
	ret->pextra=(void*)&ret[1]; // 指向了ret表示的PgHdr的 结束 地址
	memset(ret->pextra, 0, pcache->sz_extra);
	
	ret->pcache=pcache;
	ret->pgno=pgno;
	ret->flags=PGHDR_CLEAN;
}


// 释放页面的引用
void pcacheRelease(PgHdr* p)
{
	assert(p->nref>0);
	
	--(p->nref);
	if(p->nref==0)
	{
		// ???
		assert((p->flags  &  PGHDR_DIRTY)==0);
		// TODO unpin页面，页面不再使用
		global_config.sql_pcache_methods.xUnpin(p->ppage);
	}
}

// 将页面插入到 页面所在PCache的dirty链表中 TODO ？？
void pcacheMakeDirty(PgHdr *p)
{
	assert(p->nref>0);
	if(p->flags  &  PGHDR_DIRTY)
		return;
	PCache *pcache=p->pcache;
	
	p->flags |= PGHDR_DIRTY;
	if(pcache->pdirty_head)
		pcache->pdirty_head->pdirty_prev=p;
	p->pdirty_next=pcache->pdirty_head;
	p->pdirty_prev=0;
	pcache->pdirty_head=p;
}

// 获得dirty页面链表的首个指针
PgHdr *pcacheGetDirty(PCache* pcache)
{
	return pcache->pdirty_head;
}

void pcacheMakeClean(PgHdr *p)
{
	if((p->flags  &  PGHDR_DIRTY) ==0)
		return;
	PCache *pcache=p->pcache;
	
	if(p->pdirty_prev)
		p->pdirty_prev->pdirty_next=p->pdirty_next;
	if(p->pdirty_next)
		p->pdirty_next->pdirty_prev=p->pdirty_prev;
	if(pcache->pdirty_head==p)
		pcache->pdirty_head=p->pdirty_next;
	if(pcache->pdirty_tail==p)
		pcache->pdirty_tail=p->pdirty_prev;
	
	p->pdirty_next=p->pdirty_prev=0;
	p->flags ^= PGHDR_DIRTY;
}


