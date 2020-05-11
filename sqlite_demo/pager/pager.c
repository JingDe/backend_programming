#include"sqlite_demo.h"
#include"pcache.h"
#include"os.h"

/* 每个打开的文件使用一个 Pager对象管理 */
struct Pager{
	SqlVFS *pvfs; // 用来操作数据库文件的VFS
	char *filename; // 数据库文件名
	SqlFile *fd; // 数据库文件
	
	PCache* pcache; // cache模块
};

static void _pagerWritePage(PgHdr *pdirty);

/*
打开指定数据库文件的Pager对象:
分配Pager结构体，调用特定vfs的接口打开文件，创建PCache模块？？
*/
int pagerOpen(SqlVFS* pvfs, // 打开文件的vfs
		Pager** pppager, // 输出的Pager对象
		const char* filename, // 数据库文件名
		int sz_extra, // 添加到in-memroy页面后的字节数
		int flags) // 调用vfs的xOpen方法的flags
{
	Pager *ppager=0;
	void *ptr=0;
	int sz_filename=0;
	int sz_pcache=pcacheSize(); // sizeof(PCache)
	
	if(filename  &&  filename[0])
	{
		sz_filename=strlen(filename);
	}
	else
	{
		return SQL_ERROR;
	}
	
	// 数据库文件的Pager管理结构体+文件名字符串+此vfs使用的文件SqlFile结构体大小+PCache模块结构体
	ptr=malloc(sizeof(Pager)+sz_filename+1+pvfs->sz_file+sz_pcache);
	
	ppager=(Pager*)ptr;
	ppager->filename=(char*)(ptr+=sizeof(Pager));
	ppager->fd=(SqlFile*)(ptr+=(sz_filename+1));
	ppager->pcache=(PCache*)(ptr+=pvfs->sz_file);
	
	assert(filename  &&  filename[0]);
	memcpy(ppager->filename, filename, sz_filename);
	ppager->filename[sz_filename]=0;
	
	// 调用vfs层的接口，打开文件
	if(osOpen(pvfs, ppager->filename, ppager->fd, flags)==SQL_ERROR)
		return SQL_ERROR;
	
	// TODO 创建PCache模块
	if(pcacheOpen(SQL_DEFAULT_PAGE_SIZE, sz_extra, (PCache*)ppager->pcache)==SQL_ERROR)
		return SQL_ERROR;
	
	*pppager=ppager;
	return SQL_OK;
}

// PgHdr
int pagerGet(Pager* pager, Pgno pgno, DbPage** pppage)
{
	*pppage=0;
	
	// 查找页面Pager的缓存模块，是否存在指定页面，有则直接返回
	DbPage* pdb=pcacheGet(pager->pcache, pgno);
	if(pdb)
	{
		*pppage=pdb;
		return SQL_OK;
	}
	
	// 否则，获得一个新的page slot，从数据库文件中读取这一页面的内容
	pdb=pcacheFetch(pager->pcache, pgno);
	
	// 返回NULL表示没有unpinned的页面，也没有内存创建一个新的slot
	if(pdb==0)
    {
        printf("pcacheFetch return NULL\n");
        return SQL_NOMEM;
    }
	
	// 否则，成功获得一个页面，更新这个结构体
	pdb->pgno=pgno;
	pdb->pager=pager;
	pdb->flags=PGHDR_CLEAN;
	pdb->pdirty_next=0;
	pdb->pdirty_prev=0;
	
	// 读取文件的第pgno个页面的数据到页面 PgHdr.pdata 成员
	osRead(pager->fd, pdb->pdata, SQL_DEFAULT_PAGE_SIZE, SQL_DATABASE_HEADER_SIZE+(pgno-1)*SQL_DEFAULT_PAGE_SIZE);
    
    {
        // for test
        printf("pagerGet ok for page %d\n", pgno);
        char page_data[SQL_DEFAULT_PAGE_SIZE+1];
        memset(page_data, '\0', sizeof(page_data)/sizeof(page_data[0]));
        memcpy(page_data, pdb->pdata, SQL_DEFAULT_PAGE_SIZE);
        printf("[%s]\n", page_data);
    }

	*pppage=pdb;
	return SQL_ERROR;
}

int pagerUnref(DbPage* page)
{
	pcacheRelease(page);
	
	return SQL_OK;
}

int pagerWrite(DbPage* page)
{
	pcacheMakeDirty(page);
	
	return SQL_OK;
}

// 同步 Pager 的数据库文件
int pagerCommit(Pager *pager)
{
	PCache *pcache=pager->pcache;
	PgHdr *pdirty=pcacheGetDirty(pcache); // 遍历脏页链表
	
	while(pdirty)
	{
		PgHdr *next=pdirty->pdirty_next;
		
		_pagerWritePage(pdirty); // 将脏页写到数据库文件中
		pcacheMakeClean(pdirty); // 将脏页从脏页链表中删除	
		
		pdirty=next;
	}
	return SQL_OK;
}

// 将一个脏页面写到数据库文件中
static void _pagerWritePage(PgHdr *pdirty)
{
	void *pdata=pdirty->pdata; // 页面数据
	int offset=SQL_DATABASE_HEADER_SIZE + (pdirty->pgno-1) *SQL_DEFAULT_PAGE_SIZE; // offset是第pgno个页面的首地址
	// 第一个页面的pgno等于1
	int sz=SQL_DEFAULT_PAGE_SIZE;
	// 将对应页面的数据从数据库文件中读取到pager的fd中
	int ret=osWrite(pdirty->pager->fd, pdata, sz, offset);
	if(SQL_OK!=ret)
	{
		// TODO
	}
}

