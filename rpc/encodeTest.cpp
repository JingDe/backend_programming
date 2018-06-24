/*
	设计encoding_table和decding_table数组的思路：
	需求：将字符先转换成6比特，再将6比特转换成原先的字符。
	所有字符的范围包括：
			目标字符：空格(32)、数字0-9(64~73)、A-Z(65-90)、下划线(95)、a-z(97-122)。（共64个字符）（括号内是ASCII码）
			其他字符看做空格处理。
	所以第一个数组的输入包含ASCII码个数128个字符，长度为128。
	第二个数组的长度为64。
	
	从decoding_table数组开始设计。按顺序从0开始排列目标字符。得到每一个字符对应的下标。
	从而得到了在encoding_table数组中，该目标字符映射到的值，即该下标。
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

constexpr char decoding_table[] =
	" 0123456789"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
	"abcdefghijklmnopqrstuvwxyz";
	
int main()
{
	std::cout<<sizeof(encoding_table)<<", "<<sizeof(decoding_table)<<std::endl;
	
	char c=' ';
	unsigned char t=encoding_table[c];
	char res=decoding_table[t];
	std::cout<<c<<"=="<<(int)c<<" => "<<t<<" => "<<res<<std::endl;
	
	return 0;
}
