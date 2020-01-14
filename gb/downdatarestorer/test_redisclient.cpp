#include"redisclient.h"
#include<string>

using std::string;

int main(int argc, char** argv)
{
    if(argc<2)
    {
        printf("usage: %s configFileName\n", argv[0]);
        return -1;
    }
    string logConfigFileName=argv[1];
    printf("log config filename %s\n", logConfigFileName.c_str());

    OWLog::config(logConfigFileName);

    RedisClient client;
    printf("construct RedisClient ok\n");

    return 0;
}
