#include"EventLoop.h"

#include"logging.muduo/LogFile.h"

std::shared_ptr<LogFile> g_logFile;

void logOutput(const char* msg, int len)
{
	g_logFile->append(msg, len);// TODO : ÎÄ¼þ0×Ö½Ú£¿£¿
	fwrite(msg, 1, len, stdout);	
}

int main()
{
	g_logFile.reset(new LogFile("LOG_EventLoop_test", 1, false, 1));

	EventLoop loop1;
	EventLoop loop2;

	return 0;
}