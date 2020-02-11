#ifndef REDISCONNECTION_H
#define REDISCONNECTION_H

#include <string>
#include <queue>

#include "socket.h"
#include "owlog.h"
#include "redisbase.h"

#define REDIS_READ_BUFF_SIZE 4096
using namespace std;

class RedisConnection
{
public:
	enum RedisParseState {
		REDIS_PARSE_UNKNOWN_STATE,
		REDIS_PARSE_TYPE,
		REDIS_PARSE_LENGTH,
		REDIS_PARSE_ARRAYLENGTH,
		REDIS_PARSE_INTEGER,
		REDIS_PARSE_RESULT,
		REDIS_PARSE_STRING,

		// for parseScanReply
		REDIS_PARSE_CURSORLEN,
		REDIS_PARSE_CURSOR,
		REDIS_PARSE_KEYSLEN,
		REDIS_PARSE_KEYLEN,
		REDIS_PARSE_KEY,
	};
	enum ReplyParserType{
		COMMON_PARSER,
		SCAN_PARSER,
	};
	RedisConnection(const string serverIp, uint32_t serverPort, uint32_t timeout);
	~RedisConnection();
	bool connect();
//	bool doRedisCommand(list<RedisCmdParaInfo> &paraList, int32_t paraLen, RedisReplyInfo& replyInfo, ParseFunction parser=NULL);
	bool doRedisCommand(list<RedisCmdParaInfo> &paraList, int32_t paraLen, RedisReplyInfo& replyInfo, ReplyParserType parserType=COMMON_PARSER);
	bool close();
	
private:
	bool send(char* request, uint32_t sendLen);
	bool recv(RedisReplyInfo& replyInfo, ReplyParserType parserType=COMMON_PARSER);

	void checkConnectionStatus();
	bool parse(char* parseBuf, int32_t parseLen, RedisReplyInfo& replyInfo);
	bool parseScanReply(char *parseBuf, int32_t parseLen, RedisReplyInfo & replyInfo);
	
public:
	bool m_available;
	//record connection connect time.
	uint32_t m_connectTime;
private:
	string m_serverIp;
	uint32_t m_serverPort;
	uint32_t m_timeout;
	Socket m_socket;
	char*  m_unparseBuf;
	int32_t  m_unparseLen;
	int32_t  m_parseState;
	int32_t  m_arrayNum;
	int32_t  m_doneNum;
	int32_t  m_arrayLen;
	bool   m_valid;
//	OWLog	m_logger;
};

#endif
