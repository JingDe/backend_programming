#include"endian.h"

#include<stdint.h>


bool is_big_endian()
{
	union {
		uint32_t i; // 如果是小端，低字节在前（低地址）
		char c[4];
	}bint = {0x01020304};

	return bint.c[0] == 1;
}