/*
pager层，一次从文件中读或写一页，
未上层提供page cache
*/


#include"sqlite_demo.h"

#define SQL_DEFAULT_PAGE_SIZE 2048
#define SQL_DATABASE_HEADER_SIZE 0 // ???

// 表示页面编号，从1开始
typedef unsigned int Pgno;

// Pager 管理打开的文件
typedef struct Pager Pager;

// 页面的handle类型， 每一个页面通过PgHdr管理
typedef struct PgHdr DbPage;


// 打开或者关闭 一个Pager连接
int pagerOpen(SqlFile* pvfs, // 打开文件的vfs
		Pager** pppager, // 输出的Pager对象
		const char* filename, // 数据库文件名
		int sz_extra, // 添加到in-memroy页面后的字节数
		int flags) // 调用vfs的xOpen方法的flags

// 获取 页面引用
int pagerGet(Pager* pager, Pgno pgno, DbPage** pppage);

// 释放页面引用
int pagerUnref(DbPage*);

// 操作页面
int pagerWrite(DbPage*);

// 同步数据库文件
int pagerCommit(Pager*);



