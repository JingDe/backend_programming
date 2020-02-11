#ifndef THREAD_TASK_H_
#define THREAD_TASK_H_

#include "owlog.h"
#include"mutexlock.h"
#include <pthread.h>
#include <sys/types.h>
#include <linux/unistd.h>

//extern pid_t gettid();

#define gettid() syscall(__NR_gettid)

#define LOCK_GUARD OWLockGuard<OWMutexLock>
#define THREAD_COUNT_STEP		10
#define DEFAULT_STACK_SIZE		(256*1024)

class ThreadPool {	
public:
	ThreadPool();
	virtual ~ThreadPool();

	int32_t startThd(int32_t threadNum = 1, int32_t stackSize = DEFAULT_STACK_SIZE);
	int32_t increaseThd(int32_t threadNum, int32_t stackSize = DEFAULT_STACK_SIZE);
	void closeThd(void);
	
	virtual void svc(void) = 0;

	int32_t threadCount(void);
	
	virtual void join(void);
	
private:
	void createThd(int32_t threadNum, int32_t stackSize);

//protected:
//	OWMutexLock m_lockOperation; //we can't cancel thread until one operation is done

private:	
	static void* svcRun(void* args);
	static void SigHandle(int signo);
	pthread_t *m_thd_ids;
	pthread_cond_t m_cond;
	MutexLock m_lock;
	int32_t 	m_thd_count;	
	OWLog m_logger;
};


inline int32_t ThreadPool::threadCount(void) {
	return this->m_thd_count;
}

#endif /*THREAD_TASK_H_*/
