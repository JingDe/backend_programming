// 测试__thread变量哪里实例化，有几个

#include"ThreadObject.h"
#include"thread.muduo/Thread.h"

#include<iostream>

void threadFunc()
{
	ThreadObject obj2("obj2");
	std::cout << obj2.Name() << std::endl;
}

int main()
{
	//主线程创建线程对象
	ThreadObject obj1("obj1");	
	//ThreadObject obj2;
	std::cout << obj1.Name() << std::endl;

	// 线程1创建线程对象
	Thread thread(threadFunc);
	thread.start();
	thread.join();


	return 0;
}