#include"AsyncLogging.h"
#include"Logging.h"

#include<chrono>

int kRollSize = 500 * 1000 * 1000;

AsyncLogging* g_asyncLog = NULL;

void asyncOutput(const char* msg, int len)
{
	g_asyncLog->append(msg, len);
}

void bench(bool longLog)
{
	Logger::setOutput(asyncOutput);

	int cnt = 0;
	const int kBatch = 1000;
	std::string empty = " ";
	std::string longStr(3000, 'X');
	longStr += " ";

	for (int t = 0; t < 30; ++t)
	{
		for (int i = 0; i < kBatch; ++i)
		{
			LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz "
				<< (longLog ? longStr : empty)
				<< cnt;
			++cnt;
		}
		
		//struct timespec ts = { 0, 500 * 1000 * 1000 }; // 500ºÁÃë
		//nanosleep(&ts, NULL);
		std::chrono::seconds sec(1); // Ë¯Ãß1Ãë
		std::this_thread::sleep_for(sec);
	}
}

int main()
{
	AsyncLogging log("test_AsyncLogging", kRollSize);
	log.start();
	g_asyncLog = &log;

	bench(false);

	system("pause");
	return 0;
}