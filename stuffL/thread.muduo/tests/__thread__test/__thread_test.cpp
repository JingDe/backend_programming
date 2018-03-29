// 测试__thread变量哪里实例化，有几个

#include"ThreadObject.h"

#include<iostream>

void threadFunc()
{

}

int main()
{
	//主线程创建线程对象
	ThreadObject obj1("1");	
	//ThreadObject obj2;

	// 线程1创建线程对象
	Thread thread("thread1", threadFunc);

	return 0;
}