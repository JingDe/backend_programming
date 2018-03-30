#ifndef APPEND_FILE_
#define APPEND_FILE_

#include<string>

// 不是线程安全的
class AppendFile {
public:
	AppendFile(std::string filename);

	~AppendFile()
	{
		fflush(file_);
		fclose(file_);
	}

	void append(const char* buf, size_t len);

	void flush();

	size_t writtenBytes() { return writtenBytes_;  }

private:
	size_t getFileSize();

	std::string filename_;
	FILE* file_; // 销毁时，写到磁盘上，否则需要flush
	char buffer_[64 * 1024];
	size_t writtenBytes_;
};

#endif