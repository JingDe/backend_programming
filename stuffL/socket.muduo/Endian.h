#ifndef ENDIAN_H_
#define ENDIAN_H_

#include<stdint.h>

#include<endian.h>

inline uint16_t hostToNetwork16(uint16_t host16)
{
	return htobe16(host16); // 16λ������ת���ɴ����
}

inline uint32_t hostToNetwork32(uint32_t host32)
{
	return htobe32(host32);// 32λ������ת���ɴ����
}

#endif
