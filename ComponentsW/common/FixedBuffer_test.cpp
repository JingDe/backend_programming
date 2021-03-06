// logging.cpp: 定义控制台应用程序的入口点。
//

//#include "stdafx.h"


#include"FixedBuffer.h"

//#include<cassert>
#include<assert.h>
#include<iostream>
#include<string>

void testChar()
{
	char buf[] = "123";
	std::cout << sizeof(buf) << std::endl;

	std::string str = "123";
	std::cout << str.size() << std::endl;
}

// 测试append函数
void testAppend()
{
	FixedBuffer<32> smallBuffer;
	assert(smallBuffer.empty() == true);

	std::string s = "123";
	smallBuffer.append(s);
	//std::cout << smallBuffer.ToString() << std::endl;
	//std::cout << smallBuffer.size() << std::endl;
	assert(smallBuffer.ToString() == "123");

	char buf[] = "456";
	smallBuffer.append(buf, sizeof(buf)-1);
	assert(smallBuffer.ToString() == "123456");

	smallBuffer.append(buf);
	assert(smallBuffer.ToString() == "123456456");
}


int main()
{
	testChar();
	testAppend();

	system("pause");

	return 0;
}

