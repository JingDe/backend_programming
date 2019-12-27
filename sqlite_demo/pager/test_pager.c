#include"sqlite_demo.h"
#include"sqliteint.h"
#include"os.h"
#include"pager.h"
#include"pcache.h"

extern void pcache1GlobalRegister(SqlPCacheMethods *methods);

int main()
{
	// 设置 global_config.sql_pcache_methods
	pcache1GlobalRegister(&global_config.sql_pcache_methods);
	
	SqlVFS* vfs=osGetVFS("unix"); // os.c
	Pager *p;
	// 使用unix的vfs，打开数据库文件data.db，sz_extra大小是0
	pagerOpen(vfs, &p, "data.db", 0, O_RDWR);
    if(p==NULL)
    {
        printf("pagerOpen failed\n");
        return 1;
    }
	
	DbPage *page1, *page2, *page3;

    printf("FIRST GET page\n");
	pagerGet(p, 1, &page1);
	pagerGet(p, 2, &page2);
	pagerGet(p, 3, &page3);	
	printf("%p %p %p\n", page1, page2, page3);

    printf("\nWRITE and COMMIT page\n");

	pagerWrite(page1); // 在修改页面数据之前调用，将页面移动到dirty链表
	memset(page1->pdata, 'A', 1024);
	pagerCommit(p); // 将Pager的dirty链表中的所有页面写到数据库文件中，并从链表中删除
	pagerUnref(page1);
	
	pagerWrite(page2);
	memset(page2->pdata, 'B', 1024);
	pagerCommit(p);
	pagerUnref(page2);
    
	pagerWrite(page3);
//	memset(page3->pdata, 'C', SQL_DEFAULT_PAGE_SIZE);
    memset(page3->pdata, 'C', 2048);	
	pagerCommit(p);
	pagerUnref(page3);

    printf("\nRE-GET page\n");

    pagerGet(p, 1, &page1); // print page content
    pagerGet(p, 2, &page2);
    pagerGet(p, 3, &page3);
	return 0;
}
