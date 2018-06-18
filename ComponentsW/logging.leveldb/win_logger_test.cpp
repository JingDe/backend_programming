//#include"win_logger.h"
#include"Logger.h"

#include<cstdlib>

int main()
{
	Logger2 *l1;
	NewLogger("test_winlogger.txt", &l1);

	Log(l1, "hello, test_winlogger.txt");

	system("pause");
	return 0;
}