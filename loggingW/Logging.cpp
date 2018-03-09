#include"logging.h"

#include<cstdio>
#include<cstring>
#include<cstdlib>

void defaultOutput(const char* msg, int len)
{
	fwrite(msg, 1,len, stdout); 
}

void defaultFlush()
{
	fflush();
}


Logger::SourceFile::SourceFile(const char* name)
	: filename_(name)
{
	const char* slash = strrchr(name, '/');
	if (slash)
	{
		filename_ = slash + 1;
	}
	length_ = strlen(filename_);
}

template<typename N>
Logger::SourceFile::SourceFile(const char (&arr)[N])
	:filename_(arr), length_(N)
{
	const char* slash = strrchr(name, '/');
	if (slash)
	{
		filename_ = slash + 1;
		N -= static_cast<int>(filename_ - arr);
	}
}

Logger::LOG_LEVEL initLogLevel()
{
	// 根据环境变量设置

	return LOG_LEVEL::WARN;
}
//Logger::LOG_LEVEL g_loglevel = initLogLevel();


Logger::Logger(SourceFile file, int line, LOG_LEVEL level)
	:file_(file),line_(line),level_(level)
{
	setOutputFunc(defaultOutput());
	setFlushFunc(flushFunc());
}

Logger::~Logger()
{
	const LogStream::Buffer& buf = stream().buffer();
	outputFunc_(buf.data(), buf.length());
	if (level_ > LOG_LEVEL::FATAL)
	{
		flushFunc_();
		abort();
	}
}