#include"LogFile.h"
#include"Logging.h"

#include<iostream>
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
	Logger::setLogLevel(Logger::DEBUG);
	/*char name[256];
	strncpy(name, argv[0], 256);*/
	char name[256] = "test_LogFile_test.txt";
	g_logFile.reset(new LogFile(name, 100 * 1000));
	Logger::setOutput(outputFunc);
	Logger::setFlush(flushFunc);

	std::string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	for (int i = 0; i < 10000; ++i)
	{
		LOG_INFO << line << i;
		std::cout << i << std::endl;

		//usleep(1000);
		std::chrono::seconds sec(1);
		std::this_thread::sleep_for(sec);
		//thrd_sleep(&(struct timespec) { .tv_sec = 1 }, NULL); // sleep 1 sec
	}

	return 0;
}