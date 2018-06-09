#ifndef THREADMODEL_H_
#define THREADMODEL_H_

int CHECK(int state, const char* msg);

void setReuseAddr(int fd);

int setNonBlocking(int fd);

void addfd(int epfd, int fd);

#endif