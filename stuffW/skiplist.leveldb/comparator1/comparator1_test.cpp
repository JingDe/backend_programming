#include"comparator.h"
#include"common/testharness.h"

#include<iostream>

class ComparatorTest {};

TEST(ComparatorTest, t1)
{
	Comparator<int> cmp;
	std::cout << cmp.compare(3, 4) << std::endl;
}

int main()
{
	test::RunAllTests();

	system("pause");
	return 0;
}