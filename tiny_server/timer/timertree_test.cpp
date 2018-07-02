/*
g++ timertree_test.cpp timertree.cpp timer.cpp
    ../../logging.muduo/Logging.cpp ../../logging.muduo/LogStream.cpp
    ../../testharness.leveldb/testharness.cpp
    ../../thread.muduo/thread_util.cpp
    -o timertree_test.out
    -I/home/jing/myworkspace/GitSpace/practice
    -std=c++11
    -pthread
*/

#include"timertree.h"

#include"testharness.leveldb/testharness.h"
#include"logging.muduo/Logging.h"

class TimerTreeTest{};

void timerRoutine()
{
	LOG_INFO<<"timer expired";
}

TEST(TimerTreeTest, testAddDel)
{
    TimerTree tl;
	ASSERT_EQ(tl.getMinExpired(), -1);

	time_t now=time(NULL);
	Timer timer1(now+100, timerRoutine);
	tl.addTimer(&timer1);
	ASSERT_EQ(tl.getMinExpired(), now+100);

	Timer timer2(now+200, timerRoutine);
	tl.addTimer(&timer2);
	tl.debugPrint();
	ASSERT_EQ(tl.getMinExpired(), now+100);

	tl.delTimer(&timer1);
	ASSERT_EQ(tl.getMinExpired(), now+200);

	tl.delTimer(&timer2);
	ASSERT_EQ(tl.getMinExpired(), -1);
}

TEST(TimerTreeTest, testHandleExpired)
{
	TimerTree tl;
	time_t now=time(NULL);

	Timer timer1(now+100, timerRoutine);
	Timer timer2(now+200, timerRoutine);
	Timer timer3(now+300, timerRoutine);

	tl.addTimer(&timer1);
	tl.addTimer(&timer2);
	tl.addTimer(&timer3);

	tl.debugPrint();
	ASSERT_EQ(tl.getMinExpired(), now+100);

	// 50s passed
	tl.handleExpired(now+50);
	tl.debugPrint();
	ASSERT_EQ(tl.getMinExpired(), now+100);

	tl.handleExpired(now+150);
	tl.debugPrint();
	ASSERT_EQ(tl.getMinExpired(), now+200);

}

int main()
{
    Logger::setLogLevel(Logger::DEBUG);

    test::RunAllTests();

    return 0;
}
