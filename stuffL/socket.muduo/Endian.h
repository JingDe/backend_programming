#ifndef ENDIAN_H_
#define ENDIAN_H_

#include<stdint.h>

#include<endian.h>


namespace sockets
{


inline uint16_t hostToNetwork16(uint16_t host16)
{
	return htobe16(host16); // 16位主机序转换成大端序
}

inline uint32_t hostToNetwork32(uint32_t host32)
{
	return htobe32(host32);// 32位主机序转换成大端序
}

inline uint16_t networkToHost16(uint16_t net16)
{
	return be16toh(net16); // 大端序16位转换成主机序
}


}

#endif
