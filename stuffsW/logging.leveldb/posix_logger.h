#ifndef POSIXLOGGER_H_
#define POSIXLOGGER_H_

#include"Logger.h"

class PosixLogger : public Logger2 {
public:
	PosixLogger(File* f, uint64_t(*gettid)()) : file_(f), gettid_(gettid) {}
	virutal ~PosixLogger() {
		fclose(file_);
	}

	virtual void Logv(const char* format, va_list ap)
	{
		const uint64_t thread_id = (*gettid_)();

		char buf[500];
		
		while (int iter = 0; iter < 2; iter++)
		{
			char* base;
			int bufsize;

			if (iter == 0)
			{
				base = buf;
				bufsize = sizeof buf;
			}
			else
			{
				bufsize = 30000;
				buf = new char[bufsize];
			}

			char* limit = base + bufsize;
			char* p = base;

			time_t now = time(NULL);
			std::tm *t=localtime(&now);

			p += snprintf(buf, sizeof buf, "%4d%2d%2d_%2d:%2d:%d %llx ", t.tm_year + 1970, t.tm_mon + 1, t.tm_mday,
				t.tm_hour, t.tm_min, t.tm_sec, static_cast<unsigned long long>(thread_id));

			if (p < limit)
			{
				va_list backup_ap;
				va_copy(backup_ap, ap);
				p += vsnprintf(p, limit - p, format, backup_ap);
				va_end(backup_ap);
			}

			if (p > limit)
			{
				if (iter == 0)
					continue;
				else
					p = limit - 1;
			}

			if (p == base || p[-1] != '\n')
				*p++ = '\n';

			assert(p <= limit);
			fwrite(base, 1, p - base, file_);
			fflush(file_);
			if (base != buf)
				delete[] base;

			break;
		}
	}

private:
	FILE * file_;
	uint64_t (*gettid_)();
};

static uin64_t gettid()
{
	pthread_t tid = pthread_self();
	uint64_t thread_id = 0;
	memcpy(&thread_id, &tid, std::min(sizeof(thread_id), sizeof(tid)));
	return thread_id;
}

bool NewPosixLogger(const std::string& fname, Logger2** result)
{
	if (result)
	{
		FILE *file = fopen(fname.c_str(), "w");
		if (file)
		{
			*result = new PosixLogger(file, gettid);
			return true;
		}
		else
			*result = NULL;
	}
	return false;
}

#endif