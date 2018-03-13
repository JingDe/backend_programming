#ifndef LOGGING_H_
#define LOGGING_H_

#include"LogStream.h"



// Logger�ࣺ������������LogStream������ļ���
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
		template<int N> // ģ��ʹ�ú�����������һ�����ȵ�����
		SourceFile(const char (&arr)[N]); // �β�����������ã�ά�������͵�һ���֣�����N��char�����������
		const char* fileName() { return filename_;  }

	private:
		const char* filename_;
		int length_;
	};

public:
	Logger(SourceFile file, int line, LOG_LEVEL level=INFO); // ������־���ļ����кţ�
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