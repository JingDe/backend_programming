
typedef struct PgHdr PgHdr;

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


