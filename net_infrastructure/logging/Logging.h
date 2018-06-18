#ifndef LOGGING_H_
#define LOGGING_H_

#include"LogStream.h"



// Logger�ࣺ������������LogStream������ļ���
class Logger {
public:
	enum LOG_LEVEL {
		TRACE,
		DEBUG,
		INFO,
		WARN,
		ERRORL, // ?? ERROR�ѱ����壿��
		FATAL,
		NUM_LEVELS,
	};

	class SourceFile {
	public:
		SourceFile(const char* file);
		template<int N> // ģ��ʹ�ú�����������һ�����ȵ�����
		SourceFile(const char (&arr)[N]); // �β�����������ã�ά�������͵�һ���֣�����N��char�����������
		const char* fileName() { return filename_;  }

	private:
		const char* filename_;
		int length_;
	};

public:
	Logger(SourceFile file, int line, LOG_LEVEL level = INFO); // ������־���ļ����к�
	Logger(SourceFile file, int line, LOG_LEVEL level, const char* func);
	~Logger(); // ���LogStream������־�ļ��У�,�����������

	typedef void (*OutputFunc)(const char* msg, int len);
	static void setOutput(OutputFunc out);
	typedef void (*FlushFunc)();
	static void setFlush(FlushFunc flush);


	LogStream& stream() { return logStream_;  }

	// ��ú�����ȫ�ֵ���־����
	static LOG_LEVEL logLevel();
	static void setLogLevel(LOG_LEVEL);

private:
	SourceFile file_; // ��־�������ļ�
	int line_; // ��־�������ļ��е�λ��
	LOG_LEVEL level_; // ������־��Ϣ�ļ���
	LogStream logStream_; // ��־��Ϣ
	// ��־�ļ�����������������
	//OutputFunc outputFunc_;
	//FlushFunc flushFunc_;	
};


extern Logger::LOG_LEVEL g_loglevel; // ȫ�ֵ���־����

#define LOG_TRACE if(Logger::logLevel()<=Logger::TRACE) \
	Logger(__FILE__, __LINE__, Logger::TRACE, __FUNCTION__).stream()
#define LOG_DEBUG if(Logger::logLevel() <= Logger::DEBUG) \
	Logger(__FILE__, __LINE__, Logger::DEBUG, __FUNCTION__).stream()
#define LOG_INFO if(Logger::logLevel() <= Logger::INFO) \
	Logger(__FILE__, __LINE__, Logger::LOG_LEVEL::INFO).stream()
#define LOG_WARN if(Logger::logLevel() <= Logger::WARN) \
	Logger(__FILE__, __LINE__, Logger::LOG_LEVEL::WARN).stream()
#define LOG_ERROR if(Logger::logLevel() <= Logger::ERRORL) \
	Logger(__FILE__, __LINE__, Logger::LOG_LEVEL::ERRORL).stream()
#define LOG_FATAL if(Logger::logLevel() <= Logger::FATAL) \
	Logger(__FILE__, __LINE__, Logger::FATAL).stream()


template<typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char* names, T* ptr)
{
	if (ptr == NULL)
		Logger(file, line, Logger::FATAL).stream() << names;
	return ptr;
}

#define CHECK_NOTNULL(val) CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

#endif