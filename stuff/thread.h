// 线程池


#ifndef THREAD_H_
#define THREAD_H_

#include<pthread.h>

class Thread{
private:
	int no;
	pthread_t tid;
	int pipefd;
	
	bool busy;
	int connfd;
	char res[512];
	//void (*work)();
		
	//void* work(void* arg);
	
public:
	Thread();
	~Thread();
	
	void setNo(int n){no=n; }
	void setPipefd(int fd){ pipefd=fd; }
	bool isbusy(){ return busy; }
	void setbusy(bool b) {busy=b; }
	void run();
	
	friend void* work(void* arg);
};



#endif