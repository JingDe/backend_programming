#ifndef ENDIAN_H_
#define ENDIAN_H_

#include<stdint.h>

#include<endian.h>


namespace sockets
{


inline uint16_t hostToNetwork16(uint16_t host16)
{
	return htobe16(host16); // 16λ������ת���ɴ����
}

inline uint32_t hostToNetwork32(uint32_t host32)
{
	return htobe32(host32);// 32λ������ת���ɴ����
}

inline uint32_t hostToNetwork64(uint64_t host64)
{
	return htobe64(host64);// 64λ������ת���ɴ����
}

inline uint16_t networkToHost16(uint16_t net16)
{
	return be16toh(net16); // �����16λת����������
}

inline uint16_t networkToHost32(uint32_t net32)
{
	return be32toh(net32); // �����32λת����������
}

inline uint16_t networkToHost64(uint64_t net64)
{
	return be64toh(net64); // �����64λת����������
}

}

#endif
