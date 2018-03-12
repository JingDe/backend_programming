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

	std::string basename_; // �ļ���ǰ׺
	const size_t rollSize_; // ���ļ����󣬴�������־�ļ�
	const int flushInterval_; // flush�ļ����
	int checkEveryN_; // ��ÿ��append�����flush��rollFile������ÿcheckEveryN_��append����һ��
	size_t count_; // ��ǰ�Ѿ�append�Ĵ���
	
	//�ļ�����
	std::unique_ptr<AppendFile> file_;
	time_t startOfPeriod_;
	time_t lastFlush_;

	const static int kRollPerSeconds_ = 60 * 60 * 24; // 24Сʱһ����־�ļ�
};

#endif
