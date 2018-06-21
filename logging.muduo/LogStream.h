#ifndef LOGSTREAM_H_
#define LOGSTREAM_H_

#include"common/noncopyable.h"
#include"common/FixedBuffer.h"
// #include"noncopyable.h"
// #include"FixedBuffer.h"
#include<string>

const int kSmallBuffer = 4000;

// 待写数据流
class LogStream : noncopyable
{
public:
	typedef FixedBuffer<kSmallBuffer> Buffer;

public:
	LogStream & operator<<(bool v)
	{
		buffer_.append(v ? "1" : "0", 1);
		return *this;
	}

	LogStream& operator<<(short);

	LogStream& operator<<(unsigned short v);

	LogStream& operator<<(int);

	LogStream& operator<<(unsigned int v);

	LogStream& operator<<(long);

	LogStream& operator<<(unsigned long v);

	LogStream& operator<<(long long);

	LogStream& operator<<(unsigned long long v);

	LogStream& operator<<(const void* p);

	LogStream& operator<<(double v);

	LogStream& operator<<(float v)
	{
		*this << static_cast<double>(v);
		return *this;
	}

	LogStream& operator<<(char v)
	{
		buffer_.append(&v, 1);
		return *this;
	}

	LogStream& operator<<(const char* p)
	{
		buffer_.append(p, strlen(p));
		return *this;
	}

	LogStream& operator<<(std::string str)
	{
		buffer_.append(str);
		return *this;
	}

	void append(const char* data, int len) {
		buffer_.append(data, len);
	}

	const Buffer& buffer() const { return buffer_; }

	void resetBuffer() { buffer_.reset(); }

private:
	template<typename T>
	void formatInteger(T v);

	//FixedBuffer<kSmallBuffer> buffer_;
	Buffer buffer_;

	static const int kMaxNumericSize = 32; // operator<<添加的数据不超过32个字节
};

// 类Fmt功能：将数据格式化，准备输出到LogStream
class Fmt {
public:
	template<typename T>
	Fmt(const char* format, T val);

	const char* data() const { return buf_;  }
	int length() const { return length_;  }

private:
	char buf_[32];
	int length_;
};

inline LogStream& operator<<(LogStream& os, const Fmt& fmt)
{
	//os << (fmt.data(), fmt.length());
	os.append(fmt.data(), fmt.length());
	return os;
}

#endif