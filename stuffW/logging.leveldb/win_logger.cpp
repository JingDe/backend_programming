#include"win_logger.h"

#include<cstdint>
#include<windows.h>

void WinLogger::Logv(const char* format, va_list ap)
{
	/*char buf[1024];
	size_t len=static_cast<size_t>(snprintf(format, ap));
	fwrite(buf, 1, len, file_);*/

	const uint64_t thread_id = static_cast<uint64_t>(::GetCurrentThreadId());

	char buffer[500];

	for (int iter = 0; iter < 2; iter++)
	{
		char* base;
		int bufsize;
		if (iter == 0)
		{
			bufsize = sizeof(buffer);
			base = buffer;
		}
		else
		{
			bufsize = 3000;
			base = new char[bufsize];
		}

		char* p = base;
		char* limit = base + bufsize;

		SYSTEMTIME st;
		/*typedef struct _SYSTEMTIME {
			WORD wYear;
			WORD wMonth;
			WORD wDayOfWeek;
			WORD wDay;
			WORD wHour;
			WORD wMinute;
			WORD wSecond;
			WORD wMilliseconds;
		} SYSTEMTIME, *PSYSTEMTIME;*/
		// 类似struct tm; gmtime(time_t), localtime(time_t)

		::GetLocalTime(&st); // 获得当前本地日期和时间

		//通过 _TRUNCATE 将字符串截断，复制适合多字符串，保留目标缓冲区以 Null 终止，并返回成功。
		p += _snprintf_s(p, limit - p, _TRUNCATE, "%04d/%02d/%02d-%02d:%02d:%02d.%03d %llx ", 
			st.wYear, st.wMonth, st.wDay, 
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, static_cast<long long unsigned int>(thread_id));
		if (p < limit)
		{
			va_list backup_ap = ap;
			p += vsnprintf(p, limit - p, format, backup_ap); // 格式化输出va_list
			va_end(backup_ap);
		}

		if (p >= limit)
		{
			if (iter == 0)
				continue;
			else
				p = limit - 1; // truncate
		}

		if (p == base || p[-1] != '\n')
			*p++ = '\n'; 

		assert(p <= limit);
		fwrite(base, 1, p - base, file_);
		fflush(file_);
		if (base != buffer)
			delete[] base;
		break;
	}
}

bool NewLogger(const std::string& fname, Logger2** result)
{
	FILE* f = fopen(fname.c_str(), "w");
	if (f == NULL)
	{
		*result = NULL;
		return false;
	}
	else
	{
		*result = new WinLogger(f);
		return true;
	}
}