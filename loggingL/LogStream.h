#pragma once
#include"noncopyable.h"
#include"FixedBuffer.h"
#include<string>

const int kSmallBuffer = 4000;

// ��д������
class LogStream : noncopyable
{
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

	const FixedBuffer<kSmallBuffer>& buffer() { return buffer_; }

	void resetBuffer() { buffer_.reset(); }

private:
	template<typename T>
	void formatInteger(T v);

	FixedBuffer<kSmallBuffer> buffer_;

	static const int kMaxNumericSize = 32; // operator<<��ӵ����ݲ�����32���ֽ�
};