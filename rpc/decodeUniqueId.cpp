/*
	encoding_table是128长的数组，将不超过127的ASCII值转换成一个整数，该整数只需要6比特表示。范围从0到65。
	
*/

// decodes 6bit characters to ASCII
constexpr char decoding_table[] =
	" 0123456789"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
	"abcdefghijklmnopqrstuvwxyz";

inline std::string DecodeUniqueId(const uint64_t x) {
	std::string result;
	result.reserve(11);
	// don't read characters before we found the leading 0xF
	// first four bits set?
	bool read_chars = ((x & 0xF000000000000000) >> 60) == 0xF;
	uint64_t mask = 0x0FC0000000000000;
	/* mask用来读x中的6比特，要么表示一个字符，要么是001111
		mask每次右移6位，保证右边是6的整数倍比特。
	*/
	
	/*
	bitshift记录当前考虑的6比特，右边还有多少比特。每次减去6.
	*/
	for (int bitshift = 54; bitshift >= 0; bitshift -= 6, mask >>= 6) {
		if (read_chars)
			result += _detail::decoding_table[(x & mask) >> bitshift];
		else if (((x & mask) >> bitshift) == 0xF)
			read_chars = true;
	}
	return result;
}