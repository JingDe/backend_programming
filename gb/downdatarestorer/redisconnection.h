#ifndef REDISCONNECTION_H
#define REDISCONNECTION_H

#include <string>
#include <queue>

#include "socket.h"
//#include "owlog.h"
#include "redisbase.h"
#include"util.h"

#define REDIS_READ_BUFF_SIZE 4096
using namespace std;



/* for parseEnhanch, 更通用、全面的解析resp协议reply */

// 沿用 ReplyArrayInfo
//struct ArrayElem{
//	int elem_type;
//	string elem_value;
//};
//typedef vector<ArrayElem> Array;
typedef vector<ReplyArrayInfo> Array;

//struct ReplyInfo{
//	// 返回一条成功或者失败消息
//	// 返回一个整数的命令
//	// 返回一个对象，一个string对象。get
//	// 返回一个数组。 smembers, sentinel master,
//	// 返回一个整数，和一个数组。  scan
//	// 返回多个数组。 sentinel salves,
//	
//	int code; // 成功或者失败
//	string msg; // 失败msg
//	int num; // 
//	string entity; // string表示的对象
//	vector<Array> arrays;
//
//	// 记录parse过程中的中间状态
//	int arrays_size; // 获取arrays的大小
//	int cur_array_pos; // 当前解析的第几个Array
//	int cur_arry_size; // 获取当前解析的Array的大小
//};

struct ReplyInfo{
	int replyType;
	string resultString;
	int intValue;
	
//	list<ReplyArrayInfo> arrayList; // 返回一个string对象时，存储在第一个元素
	vector<Array> arrays; // 返回一个string对象时，存储在第一个Array的第一个元素
	
	// 记录parse过程中的中间状态
	int arrays_size; // 获取arrays的大小
	int cur_array_pos; // 当前解析的第几个Array
	int cur_arry_size; // 获取当前解析的Array的大小
};


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

		// TODO for parseEnance
		REDIS_PARSE_CHECK_ARRAYS_SIZE, // 等待确定第二行是否*开头
		
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
	bool recv(RedisReplyInfo& replyInfo, ReplyParserType parserType=COMMON_PARSER);

	string toString(){
		string desc="server ip="+m_serverIp+", port="+toStr(m_serverPort);
		return desc;
	}

	bool CanRelease()
	{
		return m_canRelease;
	}
	void SetCanRelease(bool canRelease);
	bool ListenMessage(int& handler);
	bool WaitMessage(int handler);
	bool StopListen(int handler);
	string GetServerIp() {
		return m_serverIp;
	}
	uint32_t GetServerPort() {
		return m_serverPort;
	}

private:
	bool send(char* request, uint32_t sendLen);
	void checkConnectionStatus();
	bool parse(char* parseBuf, int32_t parseLen, RedisReplyInfo& replyInfo);
	bool parseScanReply(char *parseBuf, int32_t parseLen, RedisReplyInfo & replyInfo);

	bool parseEnance(char *parseBuf, int32_t parseLen, ReplyInfo & replyInfo);
	
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
	bool   	m_valid;
	bool 	m_canRelease;
};

#endif
