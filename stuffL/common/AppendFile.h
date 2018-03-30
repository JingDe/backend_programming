#ifndef APPEND_FILE_
#define APPEND_FILE_

#include<string>

// �����̰߳�ȫ��
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
	FILE* file_; // ����ʱ��д�������ϣ�������Ҫflush
	char buffer_[64 * 1024];
	size_t writtenBytes_;
};

#endif