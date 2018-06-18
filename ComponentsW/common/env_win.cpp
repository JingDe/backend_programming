#include"env.h"

#include<thread>

class WinEnv : public Env {
public:
	virtual void Schedule(void(*function)(void* arg), void* arg);
};

// bug??
// 创建新线程，执行function
void WinEnv::Schedule(void(*function)(void* arg), void* arg)
{
	std::thread t(function, arg);
	//printf("thread started\n");
	//t.join();
	t.detach();
}


static Env* env;

Env* Env::Default()
{
	if (env == nullptr)
		env = new WinEnv;
	return env;
}