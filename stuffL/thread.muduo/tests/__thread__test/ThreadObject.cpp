#include"thread.muduo/thread_util.h"
#include"ThreadObject.h"

#include<cstdio>
#include<cstdlib>

__thread ThreadObject* t_objectInThread=0;

ThreadObject::ThreadObject(const std::string& name):name_(name), threadID_(gettid()) // TODO: Ԥ��һ��ϵͳ����
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