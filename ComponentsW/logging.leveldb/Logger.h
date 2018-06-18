#ifndef LOGGER_H_LEVELDB_
#define LOGGER_H_LEVELDB_

//#if defined(OS_WIN)
//#include"win_logger.h"
//#elif defined(POSIX)
//#include"posix_logger.h"
//#endif

#include<cstdarg>
#include<string>

class Logger2;

extern void Log(Logger2* info_log, const char* format, ...);

class Logger2 {
public:
	Logger2() {}
	virtual ~Logger2();

	// 格式化写入数据到log文件
	virtual void Logv(const char* format, va_list ap) = 0;

private:
	Logger2(const Logger2&);
	void operator=(const Logger2&);
};

extern bool NewLogger(const std::string& fname, Logger2** result);

#endif