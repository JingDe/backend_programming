#include"Logging.h"

#include<iostream>

void test1()
{
	LOG_INFO << "HAHA";
}

int main()
{
	Logger::setLogLevel = initLogLevel();

	test1();

	
	system("pause");

	return 0;
}