
#include"timerlist.h"

#include"logging.muduo/logging.h"
#include"testharness.leveldb/testharness.h"

class timerlistTest{};

void timerRoutine()
{
	LOG_INFO<<"timer expired";
}

TEST(timerlistTest, testAddDel)
{
	TimerList tl;
	ASSERT_EQ(tl.getMinExpired(), -1);
	
	time_t now=time(NULL);
	Timer timer1(now+100, timerRoutine);
	tl.addTimer(&timer1);	
	ASSERT_EQ(tl.getMinExpired(), now+100);
	
	Timer timer2(now+200, timerRoutine);
	tl.addTimer(&timer2);	
	ASSERT_EQ(tl.getMinExpired(), now+100);
	
	tl.delTimer(&timer1);
	ASSERT_EQ(tl.getMinExpired(), now+200);
	
	tl.delTimer(&timer2);
	ASSERT_EQ(tl.getMinExpired(), -1);
}


TEST(timerlistTest, testHandleExpired)
{
	
}

int main()
{
	test::RunAllTests();
	
	return 0;
}
