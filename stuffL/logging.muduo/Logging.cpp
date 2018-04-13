#include"logging.muduo/Logging.h"
//#include"port/port.h"
#include"thread.muduo/thread_util.h"

#include<iostream>
#include<cstdio>
#include<cstring>
#include<cstdlib>
#include<ctime>
#include<cassert>

#include<sys/time.h>

namespace {
	void defaultOutput(const char* msg, int len)
	{
		fwrite(msg, 1, len, stdout);
	}

	void defaultFlush()
	{
		fflush(stdout);
	}

	std::string formatTime()
	{
		char buf[256];
		int len;

		struct timeval tv; // 秒、微秒
		gettimeofday(&tv, NULL);
		long usec = tv.tv_usec;

		time_t t = time(NULL);
		struct tm* lt = localtime(&t);
		len=snprintf(buf, sizeof buf, "%4d%02d%02d_%02d:%02d:%02d:%06d", 
			lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec, usec);
		assert(len == 24);

		return std::string(buf, len);
	}
	
}

Logger::SourceFile::SourceFile(const char* name)
	: filename_(name)
{
	const char* slash = strrchr(name, '/');
	if (slash)
	{
		filename_ = slash + 1;
	}
	length_ = static_cast<int>(strlen(filename_));
}

template<int N>
Logger::SourceFile::SourceFile(const char (&arr)[N])
	:filename_(arr), length_(N-1)
{
	const char* slash = strrchr(arr, '/');
	if (slash)
	{
		filename_ = slash + 1;
		length_ -= static_cast<int>(filename_ - arr);
	}
}

Logger::LOG_LEVEL initLogLevel()
{
	// 根据环境变量设置
#if defined(__GNUG__)
	
		if (::getenv("MUDUO_LOG_DEBUG"))
			return Logger::DEBUG;
		else
			return Logger::INFO;
#endif

	return Logger::DEBUG;
}
Logger::LOG_LEVEL g_loglevel = initLogLevel(); // 全局的日志级别

Logger::OutputFunc g_output=defaultOutput; // 必须初始化，否则异常？？
Logger::FlushFunc g_flush=defaultFlush;

const char* levelStr[Logger::NUM_LEVELS] = {
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	"FATAL"
};

Logger::Logger(SourceFile file, int line, LOG_LEVEL level)
	:file_(file),line_(line),level_(level)
{
	// 日志格式：时间、线程id、日志级别、文件名、行号、日志信息
	logStream_ << formatTime()<<' ' << static_cast<int>(gettid()) <<' ' << levelStr[level_]<<' ';
}

Logger::~Logger()
{
	logStream_ << "  - " << file_.fileName() << ":" << line_ << '\n';
	const LogStream::Buffer& buf = stream().buffer();
	g_output(buf.data(), buf.length());
	if (level_ >= LOG_LEVEL::FATAL)
	{
		g_flush();
		abort();
	}
}

Logger::LOG_LEVEL Logger::logLevel()
{
	return g_loglevel;
}
void Logger::setLogLevel(Logger::LOG_LEVEL level)
{
	g_loglevel = level;
}

void Logger::setOutput(OutputFunc out)
{
	g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
	g_flush = flush;
}