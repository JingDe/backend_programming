#include"coding.h"
#include"common/endian.h"

void EncodeFixed32(char* buf, uint32_t value) {
	if (!is_big_endian()) {
		memcpy(buf, &value, sizeof(value));
	}
	else {
		buf[0] = value & 0xff;
		buf[1] = (value >> 8) & 0xff;
		buf[2] = (value >> 16) & 0xff;
		buf[3] = (value >> 24) & 0xff;
	}
}

void PutFixed32(std::string* dst, uint32_t value) {
	char buf[sizeof(value)];
	EncodeFixed32(buf, value);
	dst->append(buf, sizeof(buf));
}