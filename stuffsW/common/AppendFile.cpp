#include"AppendFile.h"

#include<cstdio>

AppendFile::AppendFile(std::string filename) :
	filename_(filename), writtenBytes_(0) // writtenBytes_�����ļ���ǰ��С
{
	file_ = fopen(filename_.c_str(), "a+");
	setbuf(file_, buffer_); // �����ڲ�����

	writtenBytes_ = getFileSize();
}

size_t AppendFile::getFileSize()
{
	fseek(file_, 0L, SEEK_END); // �������ļ�ĩβ
	long end = ftell(file_);
	return end;
}

void AppendFile::append(const char* buf, size_t len)
{
	//memcpy(buffer_ + writtenBytes_, buf, len);
	size_t nWritten = fwrite(buf, 1, len, file_);
	while (nWritten < len)
	{
		size_t x = fwrite(buf, 1, len - nWritten, file_);
		if (x == 0)
		{
			if (ferror(file_))
			{
				fprintf(stderr, "AppendFile::append() error");
			}
			break;
		}
		nWritten += x;
	}
	writtenBytes_ += nWritten;
}

void AppendFile::flush()
{
	fflush(file_);
}