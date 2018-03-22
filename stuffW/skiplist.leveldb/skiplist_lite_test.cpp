#include"skiplist_lite.h"
#include"common/testharness.h"

struct CMP {
	/*int compare(const int& a, int b) const
	{
	if (a < b)
	return -1;
	else if (a == b)
	return 0;
	else
	return 1;
	}*/

	int operator()(const int& a, const int& b) const {
		if (a < b) {
			return -1;
		}
		else if (a > b) {
			return +1;
		}
		else {
			return 0;
		}
	}
};

class SkipTest {};

TEST(SkipTest, EMPTY)
{
	//Comparator<int> cmp;
	//SkipList<int, Comparator<int> > slist(4, cmp);
	Comparator cmp;
	SkipList slist(4, cmp);
	slist.Display();

	slist.Insert(8);
	slist.Insert(4);
	slist.Insert(5);
	slist.Display();

	printf("end empty\n");
}

int main()
{
	test::RunAllTests();

	
	system("pause");
	return 0;
}