#include"port/port.h"

void threadFunc()
{
	printf("int threadFunc() : tid=%d\n", port::getThreadID()); // 新线程
}

void threadFunc2(int x)
{
	printf("int threadFunc2() : tid=%d, arg=%d\n", port::getThreadID(), x); // 新线程
}

class Foo {
public:
	explicit Foo(double x) :x_(x) {
	}

	void memberFunc()
	{
		printf("in memberFunc() : tid=%d, Foo::x_=%f\n", port::getThreadID(), x_);
	}

	void memberFunc2(const std::string& text)
	{
		printf("in memberFunc2() : tid=%d, Foo::x_=%f, text=%s\n", port::getThreadID(), x_, text.c_str());
	}

private:
	double x_;
};

int main()
{
	printf("main thread id=%d\n", port::getThreadID());

	port::Thread t1(threadFunc);
	t1.start();
	printf("t1.tid=%d\n", t1.tid()); // main线程
	t1.join();

	//test2
	port::Thread t2(std::bind(threadFunc2, 43));
	t2.start();
	t2.join();

	//test3
	Foo foo(23.56);
	port::Thread t3(std::bind(&Foo::memberFunc, &foo));
	t3.start();
	t3.join();

	//test4
	port::Thread t4(std::bind(&Foo::memberFunc2, &foo, std::string("hello")));
	t4.start();
	t4.join();

	system("pause");
	return 0;
}