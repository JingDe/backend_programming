#ifndef LOGGING_H_
#define LOGGING_H_

#include"LogStream.h"



// Logger类：析构函数，将LogStream输出到文件中
class Logger {
public:
	enum LOG_LEVEL {
		DEBUG,
		INFO,
		WARN,
		ERROR,
		FATAL,
		NUM_LEVELS
	};

	class SourceFile {
	public:
		SourceFile(const char* file);
		template<int N> // 模板使得函数不局限于一个长度的数组
		SourceFile(const char (&arr)[N]); // 形参是数组的引用，维度是类型的一部分，具有N个char的数组的引用
		const char* fileName() { return filename_;  }

	private:
		const char* filename_;
		int length_;
	};

public:
	Logger(SourceFile file, int line, LOG_LEVEL level=INFO); // 产生日志的文件和行号，
	~Logger(); // 输出LogStream（到日志文件中）,调用输出函数

	typedef void (*OutputFunc)(const char* msg, int len);
	static void setOutput(OutputFunc out);
	typedef void (*FlushFunc)();
	static void setFlush(FlushFunc flush);


	LogStream& stream() { return logStream_;  }

	// 获得和设置全局的日志级别
	static LOG_LEVEL logLevel();
	static void setLogLevel(LOG_LEVEL);

private:
	SourceFile file_; // 日志产生的文件
	int line_; // 日志产生的文件中的位置
	LOG_LEVEL level_; // 此条日志信息的级别
	LogStream logStream_; // 日志信息
	// 日志文件对象，用输出函数替代
	//OutputFunc outputFunc_;
	//FlushFunc flushFunc_;	
};


extern Logger::LOG_LEVEL g_loglevel; // 全局的日志级别

#define LOG_DEBUG if(Logger::logLevel() <= Logger::DEBUG) \
	Logger(__FILE__, __LINE__, Logger::DEBUG).stream()
#define LOG_INFO if(Logger::logLevel() <= Logger::INFO) \
	Logger(__FILE__, __LINE__, Logger::LOG_LEVEL::INFO).stream()
#define LOG_WARN if(Logger::logLevel() <= Logger::WARN) \
	Logger(__FILE__, __LINE__, Logger::LOG_LEVEL::WARN).stream()
#define LOG_ERROR if(Logger::logLevel() <= Logger::ERROR) \
	Logger(__FILE__, __LINE__, Logger::LOG_LEVEL::ERROR).stream()
#define LOG_FATAL if(Logger::logLevel() <= Logger::FATAL) \
	Logger(__FILE__, __LINE__, Logger::FATAL).stream()
#endif