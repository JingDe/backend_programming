#include"EventLoop.h"

#include"logging.muduo/Logging.h"
#include"logging.muduo/LogFile.h"
#include"test.leveldb/testharness.h"
#include"thread.muduo/Thread.h"
#include"thread.muduo/CurrentThread.h"

std::shared_ptr<LogFile> g_logFile;

EventLoop* g_loop=0;

void logOutput(const char* msg, int len)
{
	g_logFile->append(msg, len);// TODO : ÎÄ¼þ0×Ö½Ú£¿£¿
	fwrite(msg, 1, len, stdout);	
}

class EventLoopTest {};

TEST(EventLoopTest, TwoLoopInOneThread)
{
	EventLoop loop1;
	EventLoop loop2;
}

void ThreadFunc1()
{
	g_loop->loop();
}

TEST(EventLoopTest, LoopInAnotherThread)
{
	g_loop = new EventLoop();

	Thread thread1(ThreadFunc1);
	thread1.start();
	thread1.join();
}

int main()
{
	g_logFile.reset(new LogFile("LOG_EventLoop_test", 1, false, 1));

	LOG_INFO << "main thread = " << tid();

	test::RunAllTests();

	return 0;
}