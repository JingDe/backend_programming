#include"thread_util.h"

#include<unistd.h>
#include<sys/syscall.h>


pid_t gettid()
{
	return static_cast<pid_t>(syscall(SYS_gettid));
	// return syscall(SYS_gettid);
}
