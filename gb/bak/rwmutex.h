#ifndef DOWN_DATA_RESTORER_RWMUTEX_H_
#define DOWN_DATA_RESTORER_RWMUTEX_H_

#include<cstdint>
#include <pthread.h>

class RWMutex
{
public:
	RWMutex();
	virtual ~RWMutex();

	void setMaxOperationNumber(int number);
	
	void acquireRead(void);
	void acquireWrite(void);

	void acquireCertainWrite(int num);
	void releaseCertain(void);

	void release(void);

public:
	int32_t				m_maxOperationNumber;

protected:
	int32_t				m_refCount;
	int32_t				m_numWaitingWriters;
	int32_t				m_numWaitingReads;
	pthread_mutex_t 	m_rwLock;
	pthread_cond_t		m_waitingWriters;
	pthread_cond_t		m_waitingReads;	
};

class ReadGuard
{
public:
	ReadGuard(RWMutex& rwMutex);
	virtual ~ReadGuard();
	
private:
	RWMutex*		m_rwMutex;
};

class WriteGuard
{
public:
	WriteGuard(RWMutex& rwMutex);
	virtual ~WriteGuard();
	
private:
	RWMutex*		m_rwMutex;
};

class CertainWriteGuard
{
public:
	CertainWriteGuard(RWMutex& rwMutex, int num);
	virtual ~CertainWriteGuard();
	
private:
	RWMutex*		m_rwMutex;
};

#endif

