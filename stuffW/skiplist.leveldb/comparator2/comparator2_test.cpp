#include"comparator2.h"
#include"common/testharness.h"
#include"object.h"

#include<iostream>

class ComparatorTest {};

TEST(ComparatorTest, int)
{
	Comparator<int> cmp;
	std::cout << cmp.compare(3, 4) << std::endl;
}

TEST(ComparatorTest, double)
{
	Comparator<double> cmp;
	std::cout << cmp.compare(3.3, 4.4) << std::endl;
}

TEST(ComparatorTest, Object)
{
	Comparator<Object> cmp;
	std::cout << cmp.compare(Object(5), Object(5)) << std::endl;
}

int main()
{
	test::RunAllTests();

	system("pause");
	return 0;
}