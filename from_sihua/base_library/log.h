#ifndef __LOG_H__
#define __LOG_H__

#ifdef CXX17
#include <string_view>
#else
#include<string>
#endif

namespace base {

class CLog
{
public:
    enum class Level : uint8_t
    {
        Info = 0,
        Warnning = 1,
        Error = 2,
        Fatal = 3,
    };

public:
    static CLog& Instance();

    // ��־�ļ����� ��stdout����ʾ��־���������̨��
#ifdef CXX17
    int Init(std::string_view file);
#else
    int Init(const std::string& file);
#endif

    int Uninit(void);
    
    //int WriteLog(Level level, const char *logMsg, const char *file, int line);
#ifdef CXX17
    int WriteLog(Level level, std::string_view logMsg, const char *file, int line);
#else
    int WriteLog(Level level, const std::string& logMsg, const char *file, int line);
#endif

private:
    CLog();
    ~CLog();
};

}//namespace base

#define LOG_WRITE_INFO(msg) base::CLog::Instance().WriteLog(base::CLog::Level::Info, msg, __FILE__, __LINE__)
#define LOG_WRITE_WARNING(msg) base::CLog::Instance().WriteLog(base::CLog::Level::Warnning, msg, __FILE__, __LINE__)
#define LOG_WRITE_ERROR(msg) base::CLog::Instance().WriteLog(base::CLog::Level::Error, msg, __FILE__, __LINE__)
#define LOG_WRITE_FATAL(msg) base::CLog::Instance().WriteLog(base::CLog::Level::Fatal, msg, __FILE__, __LINE__)

#endif//__LOG_H__
