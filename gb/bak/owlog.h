#ifndef _OWLOG_H_
#define _OWLOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <string>
#include <map>
//#include "base_define.h"
#include "log4cxx/logger.h"
#include "log4cxx/xml/domconfigurator.h"

extern const char* u_logLevelName[];
using namespace log4cxx;
using namespace std;

class OWLogMgr;


class OWLog {
public:

	enum { BUF_SIZE = 1024 };
	
	enum {
		LOG_LEVEL_DEBUG = 0,
		LOG_LEVEL_INFO = 1,
		LOG_LEVEL_WARN = 2,
		LOG_LEVEL_ERROR = 3,
		LOG_LEVEL_FATAL = 4
	};	
	OWLog(const char* moduleName);
	virtual ~OWLog();
	
	static void config(const string& configFile);
	void log(int logLevel, const char* format, ...);
	void error(const char* format, ...);
	void warn(const char* format, ...);
	void info(const char* format, ...);
	void debug(const char* format, ...);
	
	int getLevel();	

	bool isDebug();
	//
	static void initLevel(int logLevel);

private:
	LoggerPtr m_logger;
	int m_bufSize;
	
	void _log(int logLevel, const char* format, va_list args);
private:
friend class OWLogMgr;
};

#endif

