// ����__thread��������ʵ�������м���

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
	//���̴߳����̶߳���
	ThreadObject obj1("obj1");	
	//ThreadObject obj2;
	std::cout << obj1.Name() << std::endl;

	// �߳�1�����̶߳���
	Thread thread(threadFunc);
	thread.start();
	thread.join();


	return 0;
}