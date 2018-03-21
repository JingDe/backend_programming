#include"Logging.h"
#include"LogFile.h"

#include<iostream>
#include<cstdio>
#include<string>
#include<memory>

FILE* g_file;
//LogFile* g_logfile;
std::unique_ptr<LogFile> g_logfile;
int g_total=0;

void test1()
{
	
	LOG_DEBUG << "debug";
	LOG_INFO << "info";
	LOG_WARN << "warn";
	LOG_ERROR << "error";
	//LOG_FATAL << "fatal";
}

void dummyOutput(const char* msg, int len)
{
	g_total += len;
	if (g_file)
	{
		fwrite(msg, 1, len, g_file);
	}
	else if (g_logfile)
	{
		g_logfile->append(msg, len);
	}
	else 
	{
		fwrite(msg, 1, len, stdout);
	}
}

void bench(const char* type)
{
	Logger::setOutput(dummyOutput);

	int n = 10* 1000;
	const bool kLongLog = false;
	std::string empty = " ";
	std::string longStr(3000, 'X');
	longStr += " ";
	for (int i = 0; i < n; ++i)
	{
		LOG_INFO << "Hello " << type
			<< (kLongLog ? longStr : empty)
			<< i;
	}
}

int main()
{
	Logger::setLogLevel(Logger::DEBUG);

	//LOG_INFO << sizeof(Logger);
	//LOG_INFO << sizeof(LogStream);
	//LOG_INFO << sizeof(Fmt);
	//LOG_INFO << sizeof(LogStream::Buffer);

	//g_file = fopen("log.txt", "a+");
	//if (g_file == NULL)
	//{
	//	std::cout << "fopen error\n";
	//}
	//bench("log log.txt");
	//fclose(g_file);

	//g_file = NULL;
	//bench("log stdout");
	
	// 使用LogFile测试
	g_file = NULL;
	//g_logfile = new LogFile("test_log_file", 1 * 1024);
	g_logfile.reset(new LogFile("test_log_file", 200 * 1024)); // 字节数
	bench("log test_log_file");

	std::cout << "total bytes = " << g_total << std::endl;

	system("pause");
	return 0;
}