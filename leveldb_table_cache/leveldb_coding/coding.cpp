/*
leveldb使用小端字节序

编码：固定编码32位，固定编码64位，变长编码32位，变长编码64位

EncodeFixed32
EncodeFixed64
DecodeFixed32
DecodeFixed64

EncodeVarint32
EncodeVarint64
GetVarint32Ptr GetVarint32PtrFallback
GetVarint64Ptr

*/

/*
包装函数：
PutFixed32
PutFixed64
PutVarint32
PutVarint64
PutLenghtPrefixedSlice

GetVarint32
GetVarint64
GetLengthPrefixedSlice

*/


/*固定编码：将32位或者64位的整数，从低位到高位的各个字节依次拷贝到一段连续内存中。
如果系统是小端存储，那么32位或者64位整数，在内存中就是低位字节在前，高位字节在后，可以直接拷贝到目标内存中
*/

void EncodeFixed32(char* buf, uint32_t value)
{
#if __BYTE_ORDER == _LITTLE_ENDIAN
	memcpy(buf, &value, sizeof(value));
#else
	buf[0]= value & 0xff;
	buf[1]= (value >> 8) & 0xff;
	buf[2]= (value >> 16) & 0xff;
	buf[3]= (value >> 24) & 0xff;
#endif
}

void EncodeFixed64(char* buf, uint64_t value)
{
#if __BYTE_ORDER == _LITTLE_ENDIAN
	memcpy(buf, &value, sizeof(value));
#else
	buf[0]= value & 0xff;
	buf[1]= (value >> 8) & 0xff;
	buf[2]= (value >> 16) & 0xff;
	buf[3]= (value >> 24) & 0xff;
	buf[4]= (value >> 32) & 0xff;
	buf[5]= (value >> 40) & 0xff;
	buf[6]= (value >> 48) & 0xff;
	buf[7]= (value >> 56) & 0xff;
#endif
}

void EncodeFixed64(char* buf, uint64_t value)
{
#if __BYTE_ORDER == _LITTLE_ENDIAN
	memcpy(buf, &value, sizeof(value));
#else
	for(int i=0; i<8; i++)
	{
		buf[i]= (value >> (i*8)) & 0xff;
	}
#endif
}

uint32_t DecodeFixed32(const char* ptr)
{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(ptr);
	
	int res=0;
	for(int i=0; i<4; i++)
	{
		res |= ( *p << (i*8));
		p++;
	}
	return res;
}

uint32_t DecodeFixed32(const char* ptr)
{
	const uint8_t* const buffer=reinterpret_cast<const uint8_t*>(ptr);
	
	if(port::kLittleEndian)
	{
		uint32_t result;
		std::memcpy(&result, buffer, sizeof(uint32_t));
		return result;
	}
	
	return (static_cast<uint32_t>(buffer[0])) |
			(static_cast<uint32_t>(buffer[1]) << 8) |
			(static_cast<uint32_t>(buffer[2]) << 16) |
			(static_cast<uint32_t>(buffer[3]) << 32);
}

uint64_t DecodeFixed64(const char* ptr)
{
	const uint8_t* const buffer=reinterpret_cast<const uint8_t*>(ptr);
	
	if(port::kLittleEndian)
	{
		uint32_t result;
		std::memcpy(&result, buffer, sizeof(uint32_t));
		return result;
	}
	
	uint32_t result=0;
	for(int i=0; i<8; i++)
	{
		uint32_t tmp = static_cast<uint32_t>(buffer[i]);
		result |= tmp << (i*8);
	}
	return result;
}

uint32_t DecodeFixed64(const char* ptr)
{
	const uint8_t* const buffer=reinterpret_cast<const uint8_t*>(ptr);
	
	if(port::kLittleEndian)
	{
		uint32_t result;
		std::memcpy(&result, buffer, sizeof(uint32_t));
		return result;
	}
	
	return  (static_cast<uint32_t>(buffer[0])) |
			(static_cast<uint32_t>(buffer[1]) << 8) |
			(static_cast<uint32_t>(buffer[2]) << 16) |
			(static_cast<uint32_t>(buffer[3]) << 24) |
			(static_cast<uint32_t>(buffer[4]) << 32) |
			(static_cast<uint32_t>(buffer[5]) << 40) |
			(static_cast<uint32_t>(buffer[6]) << 48) |
			(static_cast<uint32_t>(buffer[7]) << 56);
}


/*
变长编码：将32位整数或者64位整数，编码成变长字节，减少内存占用。
将32/64位整数从低位到高位每7位取出，存储于一个字节，若不存在剩余位，第8位置为0表示编码结束，否则为1，继续处理下一个7bit位。
*/

// 32位整数，每7位编码一个字节，最多需要5个字节
char* EncodeVarint32(char* dst, uint32_t v)
{
	unsigned char* ptr=reinterpret_cast<unsigned char*>(dst);
	static const int B=128;
	if(v < (1<<7))
	{
		*(ptr++) = v;
	}
	else if(v < (1<<14))
	{
		*(ptr++) = v | B;
		*(ptr++) = v >> 7;
	}
	else if(v < (1<<21))
	{
		*(ptr++) = v | B;
		*(ptr++) = (v >> 7) | B;
		*(ptr++) = v >> 14;
	}
	else if(v < (1<<28))
	{
		*(ptr++) = v | B;
		*(ptr++) = (v >> 7) | B;
		*(ptr++) = (v >> 14) | B;
		*(ptr++) = v >> 21;
	}
	else
	{
		*(ptr++) = v | B;
		*(ptr++) = (v >> 7) | B;
		*(ptr++) = (v >> 14) | B;
		*(ptr++) = (v >> 21) | B;
		*(ptr++) = v >> 28;
	}
	return reinterpret_cast<char*>(ptr);
}

// 64位整数，每7位编码一个字节，最多需要10个字节
char* EncodeVarint64(char* dst, uint64_t v)
{
	static const int B=128;
	unsigned char* ptr=reinterpret_cast<unsigned char*>(dst);
	while(v >= B)
	{
		*(ptr++) = (v & (B-1))| B;
		v = v >> 7;
	}
	*(ptr++) = static_cast<unsigned char>(v);
	return reinterpret_cast<char*>(ptr);
}

int VarintLength(uint64_t v)
{
	int len=1;
	while(v>=128)
	{
		v >>=7;
		len++;
	}
	return len;
}

// p指向一个包含varint32的字符串，最长5个字节，limit等于p+5。将编码前的uint32_t保存到value
const char* GetVarint32Ptr(const char* p, const char* limit, uint32_t* value)
{
	if(p<limit)
	{
		uint32_t result = *(reinterpret_cast<const unsigned char*>(p));
		if(result < 128)
		{
			*value=result;
			return p+1;
		}
	}
	return GetVarint32PtrFallback(p, limit, value);
}

const char* GetVarint32PtrFallback(const char* p, const char* limit, uint32_t* value)
{
	const unsigned char* ptr=reinterpret_cast<const unsigned char*>(p);
	uint32_t v=0;
	for(int i=0; i<5  &&  (reinterpret_cast<const char*>(ptr))<limit; i++)
	{
		uint32_t tmp= *(ptr++);
		if(tmp<128)
		{
			v |= tmp << (i*7);
			value = v;
			return (reinterpret_cast<const char*>(ptr));
		}
		else
		{
			v |= (tmp & 127) << (i*7);
		}
	}
	return NULL;
}

const char* GetVarint32PtrFallback(const char* p, const char* limit, uint32_t* value)
{
	uint32_t result=0;
	for(uint32_t shift=0; shift<=28  &&  p<limit; shift+=7)
	{
		uint32_t byte = *(reinterpret_cast<const unsigned char*>(p));
		p++;
		if(byte & 128)
		{
			result |= ((byte & 127) << shift);
		}
		else
		{
			result |= (byte << shift);
			*value=result;
			return reinterpret_cast<const char*>(p);
		}
	}
	return NULL;
}

const char* GetVarint64Ptr(const char* p, const char* limit, uint64_t* value)
{
	uint64_t result=0;
	for(uint32_t shift=0; shift<=63  &&  p<limit; shift+=7)
	{
		uint64_t byte = *(reinterpret_cast<const unsigned char*>(p));
		p++;
		if(byte  &  128)
		{
			result |= ((byte & 127) << shift);
		}
		else
		{
			result |= (byte << shift);
			*value = result;
			return reinterpret_cast<const char*>(p);
		}
	}
	return NULL;
}

