#ifndef WIN_LOGGER_H_
#define WIN_LOGGER_H_

#include"logging.leveldb/Logger.h"

#include<cstdio>
#include<cassert>

class WinLogger : public Logger2 {
private:
	FILE * file_;

public:
	explicit WinLogger(FILE* f) :file_(f) {
		assert(file_);
	}
	~WinLogger() {
		fclose(file_);
	}

	//??
	virtual void Logv(const char* format, va_list ap);
};



#endif