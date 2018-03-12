#ifndef LOGFILE_H_
#define LOGFILE_H_

#include"AppendFile.h"

#include<string>
#include<memory>

class LogFile {
public:
	LogFile(const std::string& basename, size_t rollsize, int flushInterval = 3, int checkEveryN_ = 1024);

	~LogFile();

	void append(const char* buf, int len);
	void flush();
	void rollFile();

private:
	std::string getLogfileName(std::string basename, time_t* now);

	std::string basename_; // 文件名前缀
	const size_t rollSize_; // 当文件过大，创建新日志文件
	const int flushInterval_; // flush文件间隔
	int checkEveryN_; // 不每次append都检查flush和rollFile条件，每checkEveryN_次append后检查一下
	size_t count_; // 当前已经append的次数
	
	//文件对象
	std::unique_ptr<AppendFile> file_;
	time_t startOfPeriod_;
	time_t lastFlush_;

	const static int kRollPerSeconds_ = 60 * 60 * 24; // 24小时一个日志文件
};

#endif
