/*
uniqueId(str)函数的算法： 将字符串转换成uint64_t 
	用一个整数0xF开始， 对字符串中从左到右每个字符 next_interim(当前结果, 字符的ASCII码)操作。 

	====>>> 从左到右的每个字符，按照 encoding_table转换成6bit，从左到右排列，最左边添加0xF，得到整数。 
	
	
	要求字符串长度不超过10。10个字符串转换成比特加上F共64位。 
*/



#include<iostream>

// encodes ASCII characters to 6bit encoding
constexpr unsigned char encoding_table[] = {
	/*     ..0 ..1 ..2 ..3 ..4 ..5 ..6 ..7 ..8 ..9 ..A ..B ..C ..D ..E ..F  */
	/* 0.. */  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 1.. */  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 2.. */  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	/* 3.. */  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 0,  0,  0,  0,  0,  0,
	/* 4.. */  0, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
	/* 5.. */ 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,  0,  0,  0,  0, 37,
	/* 6.. */  0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
	/* 7.. */ 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,  0,  0,  0,  0,  0 };

uint64_t next_interim(uint64_t current, std::size_t char_code) {
	uint64_t res= (current << 6) | encoding_table[(char_code <= 0x7F) ? char_code : 0];
	std::cout<<current<<", "<<char_code<<" => "<<res<<std::endl;
	return res;
}

constexpr uint64_t atom_val(const char* cstr, uint64_t interim = 0xF) {
	return (*cstr == '\0') ?
		interim :
		atom_val(cstr + 1,
			next_interim(interim, static_cast<std::size_t>(*cstr)));
}
		
int main()
{
	uint64_t result=atom_val("add");
	std::cout<<result<<std::endl;
	
	return 0;
}
