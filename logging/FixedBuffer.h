#pragma once
#ifndef FIXEDBUFFER_H_
#define FIXEDBUFFER_H_

#include"noncopyable.h"

#include<string>
#include<cstring>  // memcpy
// #include<strings.h>

template<int SIZE>
class FixedBuffer : noncopyable
{
public:
	FixedBuffer() :cur_(data_) {}
	~FixedBuffer()
	{
		// delete data_;
	}

	bool empty() { return cur_ == data_; }
	bool full() { return cur_ == end(); }

	int size() { return cur_ - data_; }
	int available() { return sizeof(data_) - size(); }

	char* current() { return cur_; }

	void add(size_t len) { cur_ += len; }

	void append(const std::string& str) {
		int len = str.size();
		if (len > available())
			len = available();
		for (int i = 0; i<len; i++)
			*(cur_++) = str[i];
	}

	void append(const char* str) { // str不修改所指向字符串
		int len = strlen(str);
		if (len > available())
			len = available();
		/*for(int i=0; i<len; i++)
		*(cur_++)=str[i];*/
		memcpy(cur_, str, len);
		cur_ += len;
	}

	void append(const char* buf, size_t len)
	{
		if (len < static_cast<size_t>(available()))
		{			
			memcpy(cur_, buf, len);
			cur_ += len;
		}
	}

	void reset() { cur_ = data_; }
	void bzero() {
		//::bzero(data_, sizeof(data_)); // bzero函数，strings.h头文件，POSIX标准
		memcpy(data_, 0, sizeof(data_));
	}


	std::string ToString() { return std::string(data_, size()); }

	const char* debugString();

private:
	const char* end() const { return data_ + sizeof data_; }

	char data_[SIZE];
	char* cur_;
};

#endif
