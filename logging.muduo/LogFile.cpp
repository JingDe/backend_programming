#include"LogFile.h"
#include"port/port.h"
#include"common/MutexLock.h"

#include<ctime>
#include<iostream>

LogFile::LogFile(const std::string& basename, size_t rollsize, bool threadSafe, int flushInterval, int checkEveryN_) :
	basename_(basename), rollSize_(rollsize), 
	mutex_(threadSafe ?  new port::Mutex  :  NULL),
	flushInterval_(flushInterval), count_(0), startOfPeriod_(0), lastFlush_(0)
{
	rollFile();
}

LogFile::~LogFile()
{
	// std::unique_ptr<AppendFile> file_; // �Զ�fflush��fclose
}

void LogFile::append(const char* buf, int len)
{
	if (mutex_)
	{
		MutexLock lock(*mutex_);
		append_unlocked(buf, len);
	}
	else
		append_unlocked(buf, len);
}

void LogFile::append_unlocked(const char* buf, int len)
{
	file_->append(buf, len);

	// ���flush��rollFile
	// �ļ�����rollFile
	// �ļ����ɣ�rollFile
	// ����flushInterval_��flush
	if (file_->writtenBytes() > rollSize_)
	{
		rollFile();
	}
	else
	{
		count_++;
		if (count_ >= checkEveryN_)
		{
			count_ = 0;
			time_t now = time(NULL); 
			time_t timeperiod = now / kRollPerSeconds_ * kRollPerSeconds_;
			if (timeperiod != startOfPeriod_)
			{
				rollFile();
			}
			else if (now - lastFlush_ > flushInterval_)
			{
				file_->flush();
				lastFlush_ = now;
			}
		}
	}
}

void LogFile::flush()
{
	if (mutex_)
	{
		MutexLock lock(*mutex_);
		file_->flush();
	}
	else
		file_->flush();
}

void LogFile::rollFile()
{
	time_t now = time(NULL);
	std::string filename = getLogfileName(basename_, &now);
	file_.reset(new AppendFile(filename)); // FIXME: ��ʱ����rollFile��Ȼ��ͬһ����־�ļ���������ִ��append����rollFile����

	time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;
	startOfPeriod_ = start;
}

std::string LogFile::getLogfileName(std::string basename, time_t* now)
{
	std::string filename = basename; // ��־�ļ�����ʱ�䡢������������id

	std::tm *t;
	t = gmtime(now); // UTC
	// tm=localtime(now); // local
	char buf[32];
	strftime(buf, sizeof buf, ".%Y%m%d-%H%M%S", t);
	filename += buf;
	
	filename += ".log";
	return filename;
}