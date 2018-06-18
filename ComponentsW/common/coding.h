#ifndef CODING_H_
#define CODING_H_

#include"port/port.h"
#include"common/endian.h"

#include<stdint.h>

inline uint32_t DecodeFixed32(const char* ptr) {
	if (!is_big_endian()) {
		// Load the raw bytes
		uint32_t result;
		memcpy(&result, ptr, sizeof(result));  // gcc optimizes this to a plain load
		return result;
	}
	else {
		return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[0])))
			| (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8)
			| (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16)
			| (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24));
	}
}


extern void EncodeFixed32(char* dst, uint32_t value);

extern void PutFixed32(std::string* dst, uint32_t value);


#endif
