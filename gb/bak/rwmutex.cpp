#include "rwmutex.h"

RWMutex::RWMutex() 
{
	m_refCount = 0;
	m_numWaitingWriters = 0;
	m_numWaitingReads = 0;
	pthread_mutex_init(&m_rwLock, NULL); 

	m_maxOperationNumber = 1;
	
	pthread_cond_init(&m_waitingWriters, NULL);
		
	pthread_cond_init(&m_waitingReads, NULL);		
}

RWMutex::~RWMutex()
{
	pthread_mutex_destroy(&m_rwLock);
	pthread_cond_destroy(&m_waitingWriters);
	pthread_cond_destroy(&m_waitingReads);
}


void RWMutex::setMaxOperationNumber(int number)
{
	if (number >=1) {
		m_maxOperationNumber = number;
	}
}

void RWMutex::acquireRead(void)
{
	::pthread_mutex_lock(&m_rwLock);

  	while(m_refCount < 0 || m_numWaitingWriters > 0)
    {
    	m_numWaitingReads++;
      	pthread_cond_wait(&m_waitingReads, &m_rwLock);
  		m_numWaitingReads--;
	}
    m_refCount++;
    
	::pthread_mutex_unlock(&m_rwLock);
}

void RWMutex::acquireWrite(void)
{
	::pthread_mutex_lock(&m_rwLock);

  	while(m_refCount != 0)
    {
    	m_numWaitingWriters++;
      	pthread_cond_wait(&m_waitingWriters, &m_rwLock);
  		m_numWaitingWriters--;
	}
	
    m_refCount = -1;
    
	::pthread_mutex_unlock(&m_rwLock);
}

void RWMutex::acquireCertainWrite(int num)
{
    ::pthread_mutex_lock(&m_rwLock);

    while(m_refCount >= num)
    {
    	m_numWaitingWriters++;
      	pthread_cond_wait(&m_waitingWriters, &m_rwLock);
  	m_numWaitingWriters--;
    }
	
    m_refCount++;
    
    ::pthread_mutex_unlock(&m_rwLock);
}

void RWMutex::releaseCertain(void)
{
	::pthread_mutex_lock(&m_rwLock);

  	if(m_refCount > 0) // releasing a reader.
  	{
    	m_refCount--;
  	}
  	else if (m_refCount == -1) // releasing a writer.
  	{
    	m_refCount = 0;
  	}
  	else
  	{
    	return;
  	}

	if (m_numWaitingWriters > 0)  {
	      	pthread_cond_signal (&m_waitingWriters);
	}

	::pthread_mutex_unlock(&m_rwLock);
}


void RWMutex::release(void)
{
	::pthread_mutex_lock(&m_rwLock);

  	if(m_refCount > 0) // releasing a reader.
  	{
    	m_refCount--;
  	}
  	else if (m_refCount == -1) // releasing a writer.
  	{
    	m_refCount = 0;
  	}
  	else
  	{
    	return;
  	}

	if( (m_numWaitingWriters > 0) && (m_refCount == 0) )
    {
      	pthread_cond_signal (&m_waitingWriters);
    }
  	else if( (m_numWaitingReads > 0) && (m_numWaitingWriters == 0) )
    {
      	pthread_cond_broadcast(&m_waitingReads);
    }

	::pthread_mutex_unlock(&m_rwLock);
}

ReadGuard::ReadGuard(RWMutex& rwMutex) 
{
	m_rwMutex = &rwMutex;
	if (m_rwMutex->m_maxOperationNumber == 1) {
		m_rwMutex->acquireRead();
	} else {
		m_rwMutex->acquireWrite();
	}		
}

ReadGuard::~ReadGuard()
{
	if (m_rwMutex->m_maxOperationNumber == 1) {
		m_rwMutex->release();
	} else {
		m_rwMutex->releaseCertain();
	}		
}

WriteGuard::WriteGuard(RWMutex& rwMutex) 
{
	m_rwMutex = &rwMutex;
	if (m_rwMutex->m_maxOperationNumber == 1) {
		m_rwMutex->acquireWrite();
	} else {
		m_rwMutex->acquireCertainWrite(1);
	}
}

WriteGuard::~WriteGuard()
{
	if (m_rwMutex->m_maxOperationNumber == 1) {
		m_rwMutex->release();
	} else {
		m_rwMutex->releaseCertain();
	}		

}

CertainWriteGuard::CertainWriteGuard(RWMutex& rwMutex, int num) 
{
	m_rwMutex = &rwMutex;
	m_rwMutex->acquireCertainWrite(num);
}

CertainWriteGuard::~CertainWriteGuard()
{
	m_rwMutex->releaseCertain();
}

