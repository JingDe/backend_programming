#include"thread.muduo/thread_util.h"
#include"ThreadObject.h"

#include<cstdio>
#include<cstdlib>

ThreadObject::ThreadObject() :threadID_(gettid()) // TODO: Ԥ��һ��ϵͳ����
{
	if (t_objectInThread)
	{
		// TODO: ʹ��logging
		printf("ThreadObject already exits.\n");
		exit(1); // cstdlib
	}
	else
		t_objectInThread = this;
}