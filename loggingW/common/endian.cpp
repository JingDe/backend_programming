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