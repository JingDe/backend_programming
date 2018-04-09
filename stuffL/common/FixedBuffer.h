#ifndef FIXEDBUFFER_H_
#define FIXEDBUFFER_H_

#include"common/noncopyable.h"

#include<string>
#include<cstring>  // memcpy


const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

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

	int length() const { return static_cast<int>(cur_ - data_); }
	int available() const { 
		// return static_cast<int>(sizeof(data_)) - length(); 
		return static_cast<int>(end()-data_);
	}

	const char* data() const { return data_;  }
	char* current() { return cur_; }

	void add(size_t len) { cur_ += len; }

	void append(const std::string& str) {
		int len = static_cast<int>(str.size());
		if (len > available())
			len = available();
		for (int i = 0; i<len; i++)
			*(cur_++) = str[i];
	}

	void append(const char* str) { // str���޸���ָ���ַ���
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
		//if (len > strlen(buf)) // ���buf�ַ�����û��null�ַ�����δ����
		//	len = strlen(buf);
		if (len > static_cast<size_t>(available()))
		{			
			len = static_cast<size_t>(available());
		}
		memcpy(cur_, buf, len);
		cur_ += len;
	}

	void reset() { cur_ = data_; }
	void bzero() {
		//::bzero(data_, sizeof(data_)); // bzero������strings.hͷ�ļ���POSIX��׼
		memset(data_, 0, sizeof(data_));
	}


	std::string ToString() const { return std::string(data_, length()); }

	const char* debugString();

private:
	const char* end() const { return data_ + sizeof data_; }

	char data_[SIZE];
	char* cur_;
};

#endif
