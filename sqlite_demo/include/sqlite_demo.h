
#ifndef SQLITE_DEMO_SQLITE_DEMO_H
#define SQLITE_DEMO_SQLITE_DEMO_H

#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<assert.h>

#include<unistd.h>
#include<time.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>


#define SQL_OK 0
#define SQL_ERROR 1
#define SQL_NOMEM 2

#define SQL_DEFAULT_FILE_CREATE_MODE 744

typedef struct SqlIOMethods SqlIOMethods;
typedef struct SqlFile SqlFile;
typedef struct SqlVFS SqlVFS;


struct SqlFile
{	
	const struct SqlIOMethods *p_methods; // 文件操作方法
};


struct SqlIOMethods{
	int (*xClose)(SqlFile*);
	int (*xRead)(SqlFile*, void* buf, int buf_sz, int offset);
	int (*xWrite)(SqlFile*, const void* content, int sz, int offset);
	int (*xTruncate)(SqlFile*, int sz);
	int (*xSync)(SqlFile*);
	int (*xFileSize)(SqlFile*, int *ret);
};

struct SqlVFS{
	int sz_file; // SqlFile("继承类")的字节数
	int max_filename; // 文件名的最大长度
	const char* vfs_name; // vfs的名称
	void* custom_data; //
	
	// 打开文件方法
	int (*xOpen)(SqlVFS*, const char* filename, SqlFile* ret, int flags);
	// 删除文件方法
	int (*xDelete)(SqlVFS*, const char* filename);
	int (*xAccess)(SqlVFS*, const char* filename, int flag, int *ret);
	int (*xSleep)(SqlVFS*, int micro_secs);
	int (*xCurrentTime)(SqlVFS*, int *ret);
};

/*
模糊类型，由pluggable模块实现。sqlite不知道其大小或内部结构。
用来传递pluggable cache模块到sqlite内核。
TODO

PCache用来实际实现page cache功能的结构体
*/
typedef struct SqlPCache SqlPCache;

/* sqlite的sqlite3_pcache_page结构体，
定义了一个页面的page cache结构体， */
typedef struct SqlPCachePage SqlPCachePage;
struct SqlPCachePage{
	void* content; // 页面内容
	void* extra; // 页面的附加信息
};

/* sqlite的sqlite3_pcache_methods2，定义了cache管理接口，
包含创建hash表、添加删除查找置换page等操作 */
typedef struct SqlPCacheMethods SqlPCacheMethods;
struct SqlPCacheMethods{
	// 函数指针的指针
	// 创建 PCache1 模块
	SqlPCache *(*xCreate)(int sz_page, int sz_extra, int mx_pages);
	// PCache1 页面查找方法
	SqlPCachePage *(*xGet)(SqlPCache*, unsigned int key);
	// 页面获取方法
	SqlPCachePage *(*xFetch)(SqlPCache*, unsigned int key);
	// unpin 一个页面，不使用这个页面
	void (*xUnpin)(SqlPCachePage*);
};

#ifdef DEBUG
#define DEBUG_PRINT(format, ...) printf()
#else
#define 

#endif
