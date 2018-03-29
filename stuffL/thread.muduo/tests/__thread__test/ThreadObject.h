#ifndef __THREAD_TEST_H_
#define __THREAD_TEST_H_

#include<sys/types.h>
#include<string>

class ThreadObject {
public:
	explicit ThreadObject(const std::string& name);
	std::string Name() { return name_; }
	void setName(const std::string& name) { name_ = name;  }

private:
	pid_t threadID_;
	std::string name_;
};


// __thread������ͬȫ�ֱ�����ͷ�ļ���extern����
//extern __thread ThreadObject* t_objectInThread;

#endif