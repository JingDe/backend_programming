// ����__thread��������ʵ�������м���

#include"ThreadObject.h"

#include<iostream>

void threadFunc()
{

}

int main()
{
	//���̴߳����̶߳���
	ThreadObject obj1("1");	
	//ThreadObject obj2;

	// �߳�1�����̶߳���
	Thread thread("thread1", threadFunc);

	return 0;
}