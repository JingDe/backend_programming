#include"LogFile.h"
#include"Logging.h"
#include<memory>
#include<thread>
#include <chrono>

std::unique_ptr<LogFile> g_logFile;

void outputFunc(const char* msg, int len)
{
	g_logFile->append(msg, len);
}

void flushFunc()
{
	g_logFile->flush();
}

int main(int argc, char* argv[])
{
	char name[256];
	strncpy(name, argv[0], 256);
	g_logFile.reset(new LogFile(name, 200 * 1000));
	Logger::setOutput(outputFunc);
	Logger::setFlush(flushFunc);

	std::string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	for (int i = 0; i < 10000; ++i)
	{
		LOG_INFO << line << i;

		//usleep(1000);
		std::this_thread::sleep_for(2);
	}
}