
/*
当被fetch的页面没有被初始化的时候，此方法执行页面初始化。

*/
static SQLITE_NOINLINE PgHdr *pcacheFetchFinishWithInit(
		PCache *pCache, // 从此cache获取页面
		Pgno pgno, // page number
		sqlite3_pcache_page *pPage) // PcacheFetch方法获取的页面
{
	PgHdr *pPgHdr;
	assert(pPage!=0);
	pPgHdr=(PgHdr*)pPage->pExtra; 
	// sqlite3_pcache_page的pExtra指向PgHdr
	
	assert(pPgHdr->pPage==0);
	memset(&pPgHdr->pDirty, 0, sizeof(PgHdr)-offsetof(PgHdr, pDirty));
	pPgHdr->pPage=pPage;
	pPgHdr->pData=pPage->pBuf;
	
	pPgHdr->pExtra=(void*)&pPgHdr[1];
	// PgHdr的pExtra指向了？？，MemPage首地址
	memset(pPgHdr->pExtra, 0, 8); 
	// 设置 MemPage的第一个 isInit变量为0，表示未初始化
	
	pPgHdr->pCache=pCache;
	pPgHdr->pgno=pgno;
	pPgHdr->flags=PGHDR_CLEAN;
	return 
}