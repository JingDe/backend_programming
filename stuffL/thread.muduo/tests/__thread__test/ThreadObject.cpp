#include"thread.muduo/thread_util.h"
#include"ThreadObject.h"

#include<cstdio>
#include<cstdlib>

ThreadObject::ThreadObject() :threadID_(gettid()) // TODO: 预先一次系统调用
{
	if (t_objectInThread)
	{
		// TODO: 使用logging
		printf("ThreadObject already exits.\n");
		exit(1); // cstdlib
	}
	else
		t_objectInThread = this;
}