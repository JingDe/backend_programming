#ifndef LOGGING_H_
#define LOGGING_H_



// Logger�ࣺ������������LogStream������ļ���
class Logger {
	enum LOG_LEVEL {
		DEBUG,
		INFO,
		WARN,
		ERROR,
		FATAL
	};

	class SourceFile {
	public:
		SourceFile(const char* file);
		template<int N> // 
		SourceFile(const char(&arr)[N]); // 

	private:
		const char* filename_;
		int length_;
	};

public:
	Logger(SourceFile file, int line, LOG_LEVEL level); // ������־���ļ����кţ�
	~Logger(); // ���LogStream������־�ļ��У�,�����������

	typedef void(*OutputFunc)(const char* msg, int len);
	void setOutput(OutputFunc out) { outputFunc_ = out;  }
	tyepdef void(*FlushFunc)();
	void setFlush(FlushFunc flush) { flushFunc_ = flush;  }


	const LogSteam& stream() { return logStream_;  }

	static LOG_LEVEL logLevel() { return level_;  }
	static void setLogLevel(LOG_LEVEL) { level_ = level;  }

private:
	SourceFile file_;
	int line_;
	LOG_LEVEL level_;
	// ��־�ļ�����������������
	OutputFunc outputFunc_;
	FlushFunc flushFunc_;
	LogStream logStream_;
};



#define LOG_INFO if(Logger::logLevel() <= Logger::INFO) \
	Logger(__FILE__, __LINE__).stream()

#endif