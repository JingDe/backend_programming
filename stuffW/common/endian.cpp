#include"endian.h"

#include<stdint.h>


bool is_big_endian()
{
	union {
		uint32_t i; // �����С�ˣ����ֽ���ǰ���͵�ַ��
		char c[4];
	}bint = {0x01020304};

	return bint.c[0] == 1;
}

// ʹ��charָ��ָ��int��ĳ���ֽ�
bool is_little_endian()
{
	int num = 1;
	if (*(char *)&num == 1)
	{
		return true;
	}
	else
	{
		return false;
	}
}


#ifdef POSIX

#include <arpa/inet.h>
bool is_big_endian2()
{
	if (htonl(47) == 47) { // �����ֽ����Ǵ��
		// Big endian
		return true;
	}
	else {
		// Little endian.
		return false;
	}
}

#endif