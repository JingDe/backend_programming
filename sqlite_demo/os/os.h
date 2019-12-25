#ifndef SQLITE_DEMO_OS_H
#define SQLITE_DEMO_OS_H

#include"sqlite_demo.h"
///////////////////////// sqlite demo

// vfs接口

int osClose(SqlFile *);
int osRead(SqlFile *, void *buf, int buf_sz, int offset);
int osWrite(SqlFile *, const void *content, int sz, int offset);
int osTruncate(SqlFile *, int sz);
int osSync(SqlFile *);
int osFileSize(SqlFile *, int *ret);


int osOpen(SqlVFS *, const char *filename, SqlFile *ret, int flags);
int osDelete(SqlVFS *, const char *filename);
int osAccess(SqlVFS *, const char *filename, int flag, int *ret);
int osSleep(SqlVFS *, int micro_secs);
int osCurrentTime(SqlVFS *, int *ret);


SqlFile *osGetFileHandle(SqlVFS *vfs);
SqlVFS *osGetVFS(const char *vfs_name);

#endif