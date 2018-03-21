#include"Logger.h"

Logger2::~Logger2()
{

}

void Log(Logger2* info_log, const char* format, ...)
{
	if (info_log)
	{
		va_list ap;
		va_start(ap, format);
		info_log->Logv(format, ap);
		va_end(ap);
	}
}

