#ifndef THREADMODEL_H_
#define THREADMODEL_H_

#include<stdint.h>

int CHECK(int state, const char* msg);

void setReuseAddr(int fd);

int setNonBlocking(int fd);

void addfd(int epfd, int fd, uint32_t evttype);

void modfd(int epfd, int fd, uint32_t evttype);

void delfd(int epfd, int fd, uint32_t evttype);

#endif