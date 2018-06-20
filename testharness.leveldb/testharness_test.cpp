#include"testharness.h"

// 1, 定义一个空类
class TestTopic{};

// 2, 使用TEST宏，注册测试函数
TEST(TestTopic, testfunc1)
{
	// 3， 测试过程中，使用ASSERT_XXX宏
	ASSERT_TRUE(3);
	
	ASSERT_EQ(3, 3);
}


int main()
{
	// 运行所有注册的（匹配环境变量的）测试函数
	test::RunAllTests();
	
	return 0;
}