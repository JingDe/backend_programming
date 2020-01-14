
#include "owlog.h"
//#include "log/owfileappender.h"

static int u_logLevel = OWLog::LOG_LEVEL_DEBUG;

void OWLog::log(int logLevel, const char* format, ...) {
	if (logLevel < LOG_LEVEL_DEBUG)
		logLevel = LOG_LEVEL_DEBUG;
	if (logLevel > LOG_LEVEL_FATAL)
		logLevel = LOG_LEVEL_FATAL;
	va_list va;
	va_start(va, format);
	_log(logLevel, format, va);
	va_end(va);
}

void OWLog::error(const char* format, ...) {
	va_list va;
	va_start(va, format);
	_log(LOG_LEVEL_ERROR, format, va);
	va_end(va);
}

void OWLog::warn(const char* format, ...) {
	va_list va;
	va_start(va, format);
	_log(LOG_LEVEL_WARN, format, va);
	va_end(va);
}

void OWLog::info(const char* format, ...) {
	/*if (getLevel() > LOG_LEVEL_INFO) {
		return;
	}*/
	if (u_logLevel > LOG_LEVEL_INFO) {
		return;
	}
	
	va_list va;
	va_start(va, format);
	_log(LOG_LEVEL_INFO, format, va);
	va_end(va);
}

void OWLog::debug(const char* format, ...) {
	/*if (getLevel() > LOG_LEVEL_DEBUG) {
		return;
	}*/
	if (u_logLevel > LOG_LEVEL_DEBUG) {
		return;
	}
	
	va_list va;
	va_start(va, format);
	_log(LOG_LEVEL_DEBUG, format, va);
	va_end(va);
}

bool OWLog::isDebug() {
	//return (m_logger->getLevel() == Level::DEBUG);
	return (m_logger->getLevel() == Level::getDebug());
}

void OWLog::_log(int logLevel, const char* format, va_list args) {
/*	//static char message[4096];
	int dataLen = 0;
	char *message = NULL;
	do {
		if (dataLen >=  m_bufSize-1) {
			m_bufSize *= 2;
		}
		if (message != NULL) {
			delete message;
		}
		message = new char[m_bufSize];
		if (message == NULL ) {
			m_logger->error(_T("malloc error when _log"));
			return;
		}
		memset(message, 0, m_bufSize);
		//two times of calling vsnprintf will lead to core on powerpc
		dataLen = vsnprintf(message, m_bufSize-1, format, args);
		//if ((n = vsnprintf(message, 4096, format, args) >= 4096)) {
		//	m_logger->warn(_T("truncating log message of 4096 bytes."));
		//}
	} while (dataLen >=  m_bufSize-1);
*/
	char message[20480];
	unsigned int dataLen = vsnprintf(message, sizeof(message)-1, format, args);
	if (dataLen >= sizeof(message)-1) {
		m_logger->warn(("truncating part of log message."));
	}	
	switch (logLevel) {
	case LOG_LEVEL_DEBUG:
		m_logger->debug((message));
		break;
	case LOG_LEVEL_INFO:
		m_logger->info((message));
		break;
	case LOG_LEVEL_WARN:
		m_logger->warn((message));
		break;
	case LOG_LEVEL_ERROR:
		m_logger->error((message));
		break;
	default:
        throw std::exception();
		//throw new OWException("wrong log level");
	}
/*
	if (message != NULL) {
		delete [] message;
		message = NULL;
	}
*/
}

void OWLog::config(const string& configFile) {
	//String f = configFile;
	//xml::DOMConfigurator::configureAndWatch(f);
        //OWFileAppender::registerClass();	
	xml::DOMConfigurator::configureAndWatch(configFile);
}

int OWLog::getLevel()
{
	if(m_logger->getParent()->getLevel() == NULL ){
		//printf("get level:level is null\n");
		return 0;
	}
	if(m_logger->getParent()->getLevel()->equals(Level::getDebug()))
		return LOG_LEVEL_DEBUG;
	else if (m_logger->getParent()->getLevel()->equals(Level::getInfo()))
		return LOG_LEVEL_INFO;
	else if (m_logger->getParent()->getLevel()->equals(Level::getWarn()))
		return LOG_LEVEL_WARN;
	else if (m_logger->getParent()->getLevel()->equals(Level::getError()))
		return LOG_LEVEL_ERROR;
	else 
		return LOG_LEVEL_FATAL;
}

OWLog::OWLog(const char* moduleName):m_logger( Logger::getLogger((moduleName))),m_bufSize(BUF_SIZE) {
      m_logger->getParent()->getLevel();
}

OWLog::~OWLog() {
}

void OWLog::initLevel(int logLevel)
{
	u_logLevel = logLevel;
	
}

