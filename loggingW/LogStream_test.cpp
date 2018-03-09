#include"LogStream.h"

#include<iostream>
#include<string>
#include<cassert>

// 测试 snprintf
void test2()
{
	char buf[256];
	snprintf(buf, sizeof buf, "%.12g", 123.4567);
	std::cout << buf << std::endl;

	snprintf(buf, sizeof buf, "%.12g", 120.123456789012345); // 小数点前后一共12位
	std::cout << buf << std::endl;
}

//测试operator<<(int)  operator<<(double)
void test1()
{
	LogStream ls;
	const LogStream::Buffer& buf = ls.buffer();

	double d = 3.123;
	ls << d;
	std::cout << buf.ToString() << std::endl;

	int i = -888;
	ls << i;
	std::cout << buf.ToString() << std::endl;
}

// 测试Boolean
void testLogStreamBooleans()
{
	LogStream os;
	const LogStream::Buffer& buf = os.buffer();

	assert(buf.ToString() == std::string(""));
	os << true;
	assert(buf.ToString() == std::string("1"));
	os << "\n";
	assert(buf.ToString() == std::string("1\n"));
	os << false;
	assert(buf.ToString() == std::string("1\n0"));

	std::cout << "passed boolean\n";
}

void testLogStreamIntegers()
{
	LogStream os;
	const LogStream::Buffer& buf = os.buffer();
	assert(buf.ToString() == std::string(""));

	os << 1;
	assert(buf.ToString() == std::string("1"));
	os << 0;
	assert(buf.ToString() == std::string("10"));
	os << -1;
	assert(buf.ToString() == std::string("10-1"));
	os.resetBuffer();

	os << 0 << " " << 123 << 'x' << 0x64;
	assert(buf.ToString() == std::string("0 123x100"));

	std::cout << "passed integers" << std::endl;
}



void testLogStreamIntegerLimits()
{
	LogStream os;
	const LogStream::Buffer& buf = os.buffer();

	os << -2147483647;
	assert(buf.ToString() == std::string("-2147483647"));

	os << static_cast<int>(-2147483647 - 1);
	assert(buf.ToString() == std::string("-2147483647-2147483648"));

	os << ' ';
	os << 2147483647;
	assert(buf.ToString() == std::string("-2147483647-2147483648 2147483647"));
	os.resetBuffer();

	os << std::numeric_limits<int16_t>::min();
	assert(buf.ToString() == std::string("-32768"));
	os.resetBuffer();

	os << std::numeric_limits<int16_t>::max();
	assert(buf.ToString() == std::string("32767"));
	os.resetBuffer();

	os << std::numeric_limits<uint64_t>::max();
	assert(buf.ToString() == std::string("18446744073709551615"));
	os.resetBuffer();

	int16_t a = 0;
	int32_t b = 0;
	int64_t c = 0;
	os << a;
	os << b;
	os << c;
	assert(buf.ToString() == std::string("000"));

	std::cout << "passed integer limits" << std::endl;
}

void testLogStreamFloats()
{
	LogStream os;
	const LogStream::Buffer& buf = os.buffer();

	os << 0.0;
	assert(buf.ToString() == std::string("0"));
	os.resetBuffer();

	os << 1.0;
	assert(buf.ToString() == std::string("1"));
	os.resetBuffer();

	os << 0.1;
	assert(buf.ToString() == std::string("0.1"));
	os.resetBuffer();

	double a = 0.1;
	os << a;
	assert(buf.ToString() == std::string("0.1"));
	os.resetBuffer();

	double b= 0.05;
	os << b;
	assert(buf.ToString() == std::string("0.05"));
	os.resetBuffer();

	double c = 0.15;
	os << c;
	assert(buf.ToString() == std::string("0.15"));
	os.resetBuffer();

	os << a+b;
	assert(buf.ToString() == std::string("0.15"));
	os.resetBuffer();

	std::cout << a + b << "\t" << c << std::endl;
	assert(a + b != c); // ??

	os << 1.23456789;
	assert(buf.ToString() == std::string("1.23456789"));
	os.resetBuffer();

	os << -1.23456789;
	assert(buf.ToString() == std::string("-1.23456789"));
	os.resetBuffer();

	std::cout << "passed floats" << std::endl;
}

void testLogStreamVoid()
{
	LogStream os;
	const LogStream::Buffer& buf = os.buffer();

	os << static_cast<void*>(0);
	assert(buf.ToString()==std::string("0x0"));
	os.resetBuffer();

	os << reinterpret_cast<void*>(8888);
	assert(buf.ToString() == std::string("0x22B8"));

	std::cout << "passed void" << std::endl;
 }

void testLogStreamStrings()
{
	LogStream os;
	const LogStream::Buffer& buf = os.buffer();

	os << "hello";
	assert(buf.ToString() == std::string("hello"));

	std::string s = " wangjing";
	os << s;
	assert(buf.ToString() == std::string("hello wangjing"));

	std::cout << "passed strings" << std::endl;
}

void testLogStreamLong()
{
	LogStream os;
	const LogStream::Buffer& buf = os.buffer();
	for (int i = 0; i < 399; ++i)
	{
		os << "123456789 ";
		assert(buf.length()==10 * (i + 1));
		assert(buf.available() == 4000 - 10 * (i + 1));
	}
	std::cout << buf.length() << "\t" << buf.available() << std::endl;

	/*char p[] = "abcdefghi ";
	std::cout << strlen(p) << std::endl;*/

	os << "abcdefghi";
	std::cout << buf.length() << "\t" << buf.available() << std::endl;
	assert(buf.length() == 3999);
	assert(buf.available() == 1);

	std::cout << "passed long" << std::endl;
}

void testLogStreamFmts()
{
	LogStream os;
	const LogStream::Buffer& buf = os.buffer();

	os << Fmt("%4d", 2);
	assert(buf.ToString() == std::string("   2"));
	os.resetBuffer();

	os << Fmt("%4.2f", 2.3); // 最小宽度4，不足补0或空格
	// 精度2,小数部分位数最多2
	assert(buf.ToString() == std::string("2.30"));
	os.resetBuffer();

	os << Fmt("%4.2f", 1.2) << Fmt("%4d", 43);
	assert(buf.ToString() == std::string("1.20  43"));

	std::cout << "passed Fmt" << std::endl;
}


int main()
{
	//test1();
	test2();

	testLogStreamBooleans();
	testLogStreamIntegers();
	testLogStreamIntegerLimits();
	testLogStreamFloats();
	testLogStreamVoid();
	testLogStreamStrings();
	testLogStreamLong();

	testLogStreamFmts();

	system("pause");
	return 0;
}
