#include"../Logging.h"

int main()
{
	Logger::setLogLevel(Logger::DEBUG);
	
	LOG_DEBUG<<"hello";
	
	return 0;
	
}
