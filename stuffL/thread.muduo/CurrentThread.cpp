#include"CurrentThread.h"


pid_t t_cachedPid=0;

void cacheTid()
{
	t_cachedPid = gettid();
}

pid_t tid()
{
	if (__builtin_expect(t_cachedPid == 0, 0)) // Ö´ÐÐ¸ÅÂÊÐ¡
		cacheTid();
	return t_cachedPid;
}