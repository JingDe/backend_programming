
#include"os.h"
#include"sqlite_demo.h"

void osTest()
{
	SqlVFS *os=osGetVFS("unix");
	SqlFile *file=osGetFileHandle(os); // 分配一个 SqlFile 结构体
	printf("%d\n", osOpen(os, "data", file, O_RDWR | O_CREAT));	
	printf("%d\n", osWrite(file, "123", 3, 0));
	printf("%d\n", osWrite(file, "abcd", 4, 1));
    {
        char buf[20];
        int ret=osRead(file, buf, 20, 0);
        printf("%d\n", ret);
        buf[ret]=0;
        printf("%s\n", buf);
    }
	printf("%d\n", osTruncate(file, 2));
	
	char buf[20];
	int ret=osRead(file, buf, 20, 0);
	printf("%d\n", ret);
	buf[ret]=0;
	printf("%s\n", buf);
}

int main()
{
	osTest();
	
	return 0;
}
