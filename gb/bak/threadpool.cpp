
#include "threadpool.h"
#include"mutexlockguard.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>
#include<stdexcept>
//_syscall0(pid_t,gettid)
/*
pid_t gettid(void) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
        : "=a" (__res) \
        : "0" (__NR_##gettid)); \
__syscall_return(pid_t,__res); \
}*/

ThreadPool::ThreadPool() :
       m_thd_ids(NULL),	m_thd_count(0), m_logger("clib.ThreadPool") {
	if(pthread_cond_init(&m_cond, NULL) != 0){
		throw std::runtime_error("init thread cond error");
	}
}

ThreadPool::~ThreadPool() {
	closeThd();
	pthread_cond_destroy(&m_cond);
}

void ThreadPool::createThd(int32_t threadNum, int32_t stackSize)
{
	if( threadNum <= 0 )
		return;
		
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, stackSize);
	pthread_attr_setguardsize(&attr, 10*1024);
	int32_t iRet = 0;
	for (int i=0; i<threadNum; i++) {
		iRet = ::pthread_create(&m_thd_ids[i], &attr, ThreadPool::svcRun, (void *) this);
		if (0 != iRet) {
			m_logger.error("pthread_create failed: %m");
			throw std::runtime_error("ThreadTask::start_thd, create thread failed!");
		} else {
			m_thd_count ++;
            ::pthread_detach(m_thd_ids[i]);
		}
               
		m_logger.info("Create thread succ! threadid = %x, m_thd_count=%d", (u_long)m_thd_ids[i], m_thd_count);
	}
}

int32_t ThreadPool::startThd(int32_t threadNum, int32_t stackSize) {
	// set the guard size to debug which thread running out of stack
	/*struct sigaction act;
	act.sa_handler = ThreadPool::SigHandle;
	act.sa_flags = 0;
	if(sigprocmask(0, NULL, &(act.sa_mask)) != 0)
	{
		throw OWException("ThreadTask", "startThd", "sigprocmask failed!");
	}
	if(sigaction(SIGSEGV, &act, NULL) != 0)
	{
		throw OWException("ThreadTask", "startThd", "sigaction failed!");
	}
	m_logger.debug("ThreadPool: signal handle succ");
	*/
	m_logger.debug("ThreadPool: not catch SIGSEGV!");

	MutexLockGuard guard(&m_lock);
	m_thd_ids = new pthread_t[threadNum];
	if (m_thd_ids == NULL) {
		throw std::runtime_error("ThreadTask::start_thd, new thd ids failed!");
	}
	//create thd
	createThd(threadNum, stackSize);
	
	return m_thd_count;
}

int32_t ThreadPool::increaseThd(int32_t threadNum, int32_t stackSize)
{
	if( threadNum <= 0 )
		return m_thd_count;
		
	MutexLockGuard guard(&m_lock);
	
	if( m_thd_ids == NULL )
		throw std::runtime_error("ThreadTask::increaseThd, thd ids point is null!");
	
	pthread_t* oldThdIds = m_thd_ids;
	int32_t oldThdCount = m_thd_count;
	
	m_thd_ids = new pthread_t[threadNum+oldThdCount];
	if (m_thd_ids == NULL) {
//		throw OWException("ThreadTask", "increaseThd", "ThreadTask::increaseThd, new thd ids failed!");
		throw std::runtime_error("ThreadTask::increaseThd, new thd ids failed!");
	}
	
	createThd(threadNum, stackSize);
	
	//copy old thread
	for(int32_t i=0; i<oldThdCount; i++)
	{
		m_thd_ids[threadNum+i] = oldThdIds[i];
	}
	
	delete []oldThdIds;
	
	return m_thd_count;
}

void* ThreadPool::svcRun(void* args) {
	OWLog logger("clib.threadpool");
	logger.info("new thread created: thread_id = %d, pid = %d", gettid(), getpid());
	try {	
		ThreadPool* pTask = (ThreadPool*)args;

		pTask->svc();
		
		MutexLockGuard guard(&(pTask->m_lock));
		pTask->m_thd_count --;
		if(pTask->m_thd_count == 0){
			if( pTask->m_thd_ids != NULL )
				delete[] pTask->m_thd_ids;
				
			::pthread_cond_broadcast(&(pTask->m_cond));
		}
		logger.info("%s", "Thread has stoped!");
		return NULL;
	}
	catch (std::exception &e) {
		logger.error("OWException: %s\n", e.what());
		throw;
	}
}

void ThreadPool::closeThd() {
	//LOCK_GUARD guardOperation(m_lockOperation);
	MutexLockGuard guard(&m_lock);	
	if (m_thd_ids == NULL || m_thd_count == 0)
		return;

	int iRet = 0;
	for (int i=0; i<m_thd_count; i++) {
		iRet = ::pthread_cancel(m_thd_ids[i]);
		if (0 != iRet) {
			m_logger.error("cancel thread(%lu) failed!", (u_long)(m_thd_ids[i]));
		} else {
			m_logger.info("closed thread(%d)", (u_long)(m_thd_ids[i]));
		}
               
	}
        sleep(m_thd_count / 50);
	//for(int i=0;i<m_thd_count;i++){

	//	int pthread_kill_err;
	//	pthread_kill_err = pthread_kill(m_thd_ids[i],0);

	//	if(pthread_kill_err == ESRCH)
	//		m_logger.info("ID.%x has exit.\n",(u_long)m_thd_ids[i]);
	//	else if(pthread_kill_err == EINVAL)
	//		m_logger.info("Send singnal to thread fail.\n");
	//	else{
	//		m_logger.info("ID.%x still alive and wait for it one second again!\n",(u_long)m_thd_ids[i]);
	//		sleep(1);
	//	}
	//}
	delete[] m_thd_ids;
	m_logger.info("%d threads closed", m_thd_count);
	m_thd_ids = NULL;
	m_thd_count = 0;
	::pthread_cond_broadcast(&m_cond);

	//m_logger.info("after pthread_cond_broadcast in thread(%d)", gettid());
}

void ThreadPool::join() {
	m_logger.debug("ThreadPool::join()");
	MutexLockGuard guard(&m_lock);
	while (m_thd_count > 0)	{
		m_logger.info("m_thd_count=%d, enter pthread_cond_wait in thread(%d)\n",  m_thd_count, gettid());
		::pthread_cond_wait( &m_cond, m_lock.mutex());	
	}
	m_logger.info("m_thd_count = %d, exit ThreadPool::join()\n", m_thd_count);
}

/*
 * Function:    static void ThreadPool::SigHandle(int)
 * Input:       int signo - signal number
 * Output:      void
 * Creator:     Yan CHENG
 * Date:        2008.08.05 14:30
 * Description: Used for handling the SIGSEGV signal.
 *
 * We set the guard size before spawning any threads.
 * So if any of the threads runs into the guard stack,
 * OS will generate SIGSEGV to that thread.
 * This function is for debugging which thread runs
 * into the guard area.
 */
void ThreadPool::SigHandle(int signo)
{
	// only works for SIGSEGV
	if(signo != SIGSEGV) return;
	// logging it down
	OWLog logger("clib.thread.threadpool");
	// there is a bug:
	// I print pthread id as an integer
	// it is non portable, only works for Linux
	logger.error("ThreadPool: error: SIGSEGV. thread %d run in guard area\n", gettid());
	// I think it is serious, so call abort() to core dump.
	abort();
}

