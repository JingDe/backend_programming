
#include"LogStream.h"
#include<iostream>

//≤‚ ‘operator<<(int)  operator<<(double)
void test1()
{
	std::cout << "test1\n";
}

int main()
{
	std::cout << "LogStream_test\n";

	test1();

	return 0;
}