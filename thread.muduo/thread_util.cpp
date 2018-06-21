#include"thread_util.h"

#include<unistd.h>
#include<algorithm>
#include<cstring>

#include<pthread.h>
#include<sys/syscall.h>


pid_t gettid()
{
	return static_cast<pid_t>(syscall(SYS_gettid));
	// return syscall(SYS_gettid);
}


uint64_t gettid2()
{
	pthread_t tid = pthread_self();
	uint64_t thread_id = 0;
	memcpy(&thread_id, &tid, std::min(sizeof(thread_id), sizeof(tid)));
	return thread_id;
}