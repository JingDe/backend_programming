#ifndef SQLITE_DEMO_PCACHE_H
#define SQLITE_DEMO_PCACHE_H

#include"pager.h"

typedef struct PgHdr PgHdr;
typedef struct PCache PCache;

/*
cache中的每一页通过 PgHdr 管理
*/
struct PgHdr{
	SqlPCachePage *ppage; // plugin pcache模块handle ?? TODO
	void *pdata; // 页面数据
	void *pextra; // extra数据
	Pager* pager; // 管理的Pager对象
	Pgno pgno; // 页面编号
	int flags; //
	PgHdr *pdirty_next; // 脏页链表的下一个节点
	PgHdr *pdirty_prev; // 
	
	// 以下为私有数据，只能被PCache模块访问
	int nref; // 页面的引用数
	PCache *pcache; // 管理的 PCache模块 
};

#define PGHDR_CLEAN 0x001
#define PGHDR_DIRTY 0x002

#define MAX_PAGES_IN_CACHE 4096

int pcacheSize();

// PCache 模块接口
int pcacheOpen(int sz_page, // 每个页面的大小
        int sz_extra, // 每个页面的额外空间
        PCache* pcache); // 预分配的 PCache

PgHdr* pcacheGet(PCache *pcache, Pgno pgno);

PgHdr *pcacheFetch(PCache* pcache, Pgno pgno);

void pcacheRelease(PgHdr* p);

PgHdr *pcacheGetDirty(PCache* pcache);

void pcacheMakeDirty(PgHdr *p);

void pcacheMakeClean(PgHdr *p);

#endif
