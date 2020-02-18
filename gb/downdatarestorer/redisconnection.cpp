#include "redisconnection.h"
#include"glog/logging.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

typedef bool (*ParseFunction)(void *parseBuf, int32_t parseLen, RedisReplyInfo & replyInfo);

RedisConnection::RedisConnection(const string serverIp,uint32_t serverPort,uint32_t timeout)//:m_logger("clib.redisconnection")
{
	m_serverIp = serverIp;
	m_serverPort = serverPort;
	m_timeout = timeout;
	m_unparseBuf = NULL;
	m_unparseLen = 0;
	m_parseState = REDIS_PARSE_UNKNOWN_STATE;
	m_valid = false;
	m_arrayNum = 0;
	m_doneNum = 0;
	m_arrayLen = 0;
	m_canRelease=true;
}

RedisConnection::~RedisConnection()
{
	m_socket.close();
}

void RedisConnection::SetCanRelease(bool canRelease)
{
	m_canRelease=canRelease;
}

bool RedisConnection::connect()
{
	if (!m_socket.connect(m_serverIp, m_serverPort))
	{
//		m_logger.warn("connection:[%p] connect to server:[%s:%u] failed.", this, m_serverIp.c_str(), m_serverPort);
		LOG(ERROR)<<"connection:["<<this<<"] connect to server:["<<m_serverIp<<":"<<m_serverPort<<"] failed.";
		return false;
	}	
	m_socket.setSoTimeout(m_timeout);
	return true;
}

bool RedisConnection::close()
{
	m_socket.close();
	return true;
}

bool RedisConnection::doRedisCommand(list < RedisCmdParaInfo > & paraList,int32_t paraLen,RedisReplyInfo & replyInfo, ReplyParserType parserType)
{
	checkConnectionStatus();
	if (m_socket.fd == INVALID_SOCKET_HANDLE)
	{
//		m_logger.warn("connection:[%p] socket may be closed by peer.", this);
		LOG(ERROR)<<"connection:["<<this<<"] socket may be closed by peer.";
		//need connect again.
		if(!connect())
		{
//			m_logger.warn("connection:[%p] reconnect to server failed.", this);
			LOG(ERROR)<<"connection:["<<this<<"] reconnect to server failed.";
			return false;
		}
	}
	char* commandBuf = NULL;
	commandBuf = (char*)malloc(paraLen);
	memset(commandBuf, 0, paraLen);
	int32_t cmdLen = 0;
	createRedisCommand(paraList, &commandBuf, cmdLen);
//	m_logger.debug("connection:[%p] send redis command:[%s] to server:[%s:%u].", this, commandBuf, m_serverIp.c_str(), m_serverPort);
//	LOG(INFO)<<"connection:["<<this<<"] send redis command:["<<commandBuf<<"] to server:["<<m_serverIp<<":"<<m_serverPort<<"].";
	if(!send(commandBuf, cmdLen))
	{
//		m_logger.warn("connection:[%p] send command:[%s] to redis:%s:%u failed.", this, commandBuf, m_serverIp.c_str(), m_serverPort);
		LOG(ERROR)<<"connection:["<<this<<"] send command:["<<commandBuf<<"] to redis:"<<m_serverIp<<":"<<m_serverPort<<" failed.";
		free(commandBuf);
		commandBuf = NULL;
		return false;
	}
	free(commandBuf);
	commandBuf = NULL;
	return recv(replyInfo, parserType);
}

bool RedisConnection::send(char * request,uint32_t sendLen)
{
	return m_socket.writeFull((void*)request, sendLen);
}

bool RedisConnection::ListenMessage(int& handler)
{
	return m_socket.WatchReadEvent(handler);
}

bool RedisConnection::WaitMessage(int handler)
{
	if(m_socket.WaitReadEvent(handler))
	{
		return true;
	}
	else
	{
		if (m_socket.fd == INVALID_SOCKET_HANDLE)
		{
			LOG(ERROR)<<"connection:["<<this<<"] socket may be closed by peer.";
			if(!connect())
			{
				LOG(ERROR)<<"connection:["<<this<<"] reconnect to server failed.";
				return false;
			}
			else
			{
				LOG(INFO)<<"connect to server ok :["<<m_serverIp<<":"<<m_serverPort<<"]";
			}
		}
		return false;
	}
}

bool RedisConnection::StopListen(int handler)
{
	return m_socket.UnWatchEvent(handler);
}

bool RedisConnection::recv(RedisReplyInfo & replyInfo, ReplyParserType parserType)
{
	//init parse data
	m_unparseBuf = NULL;
	m_unparseLen = 0;
	m_parseState = REDIS_PARSE_UNKNOWN_STATE;
	m_valid = false;
	m_arrayNum = 0;
	m_doneNum = 0;
	m_arrayLen = 0;
	//recv data.
	char recvBuf[REDIS_READ_BUFF_SIZE];
	char *toParseBuf = NULL;
	int32_t mallocLen = 0;
	int32_t toParseLen = 0;
	toParseBuf = (char*)malloc(REDIS_READ_BUFF_SIZE);
	memset(toParseBuf, 0, REDIS_READ_BUFF_SIZE);
	toParseLen += REDIS_READ_BUFF_SIZE;
	mallocLen = toParseLen;
	while(!m_valid)
	{
		memset(recvBuf, 0, REDIS_READ_BUFF_SIZE);
		int32_t recvLen = m_socket.read(recvBuf, REDIS_READ_BUFF_SIZE-1, 3);
		if (recvLen < 0)
		{
//			m_logger.warn("connection:[%p] failed to read socket", this);
			LOG(ERROR)<<"connection:["<<this<<"] failed to read socket";
			if (m_unparseBuf != NULL)
			{
				free(m_unparseBuf);
				m_unparseBuf = NULL;
			}
			if (toParseBuf != NULL)
			{
				free(toParseBuf);
				toParseBuf = NULL;
			}
			return false;
		}
		toParseLen = m_unparseLen + recvLen;
		if (m_unparseLen != 0)
		{
			if (m_unparseLen + recvLen >= mallocLen)
			{
				char *newBuf = (char*)malloc(mallocLen*2);
				memset(newBuf, 0, mallocLen*2);
				mallocLen *= 2;
				memcpy(newBuf, m_unparseBuf, m_unparseLen);
				memcpy(newBuf + m_unparseLen, recvBuf, recvLen);
				free(toParseBuf);
				toParseBuf = NULL;
				free(m_unparseBuf);
				m_unparseBuf = NULL;
				m_unparseLen = 0;
//				toParseBuf = (char*)malloc(mallocLen);
//				memset(toParseBuf, 0, mallocLen);
//				memcpy(toParseBuf, newBuf, toParseLen);
//				free(newBuf);
                toParseBuf=newBuf;				
				newBuf = NULL;
			}
			else
			{
				memset(toParseBuf, 0, mallocLen);
				memcpy(toParseBuf, m_unparseBuf, m_unparseLen);
				memcpy(toParseBuf + m_unparseLen, recvBuf, recvLen);
				free(m_unparseBuf);
				m_unparseBuf = NULL;
				m_unparseLen = 0;
			}
		}
		else
		{
			memset(toParseBuf, 0, mallocLen);
			memcpy(toParseBuf, recvBuf, recvLen);
		}
		if(parserType==SCAN_PARSER)
			parseScanReply(toParseBuf, toParseLen, replyInfo);
		else
			parse(toParseBuf, toParseLen, replyInfo);
	}
	if (m_unparseBuf != NULL)
	{
		free(m_unparseBuf);
		m_unparseBuf = NULL;
	}
	if (toParseBuf != NULL)
	{
		free(toParseBuf);
		toParseBuf = NULL;
	}
	return m_valid;
}

bool RedisConnection::parseScanReply(char *parseBuf, int32_t parseLen, RedisReplyInfo & replyInfo)
{
//	m_logger.debug("parseScanReply, connection:[%p] start to parse redis response:[%s], parseLen:%d.", this, parseBuf, parseLen);
	LOG(INFO)<<"parseScanReply, connection:["<<this<<"] start to parse redis response:["<<parseBuf<<"], parseLen: "<<parseLen;
	const char * const end = parseBuf + parseLen;
	char *p=NULL;
	char buf[256]; // enough to contain key
	if (m_parseState == REDIS_PARSE_UNKNOWN_STATE && parseBuf != NULL)
	{
		m_parseState = REDIS_PARSE_TYPE;
	}
	while(parseBuf < end)
	{
		if(m_parseState==REDIS_PARSE_TYPE)
		{
			switch(*parseBuf)
			{
				case '-':
					m_parseState = REDIS_PARSE_RESULT;
					replyInfo.replyType = RedisReplyType::REDIS_REPLY_ERROR;
//					m_logger.debug("reply ERR");
					LOG(WARNING)<<"reply ERR";
					parseBuf++;
					break;
				case '*': // assert *2
					m_parseState = REDIS_PARSE_ARRAYLENGTH;
					replyInfo.replyType = RedisReplyType::REDIS_REPLY_ARRAY;
//					m_logger.debug("reply ok");
					LOG(INFO)<<"reply ok";
					parseBuf++;
					break;
				default:
//					m_logger.error("parse type error, %c, %s", *parseBuf, parseBuf);
					LOG(ERROR)<<"parse type error, "<<*parseBuf<<", "<<parseBuf;
					m_valid=true;
					return false;
			}
		}
		else if(m_parseState==REDIS_PARSE_RESULT)
		{			
			p = strstr(parseBuf, "\r\n");
			if (p != NULL) // get full error msg
			{
				memset(buf, 0, sizeof(buf));
				memcpy(buf, parseBuf, p-parseBuf);
				replyInfo.resultString = buf; // get error msg
//				m_logger.debug("parse get ERR :%s", buf);
				LOG(INFO)<<"parse get ERR: "<<buf;
				m_valid = true;
				return true;
			}
			else
			{
				goto check_buf;
			}
		}
		else if(m_parseState==REDIS_PARSE_ARRAYLENGTH)
		{
			p = strstr(parseBuf, "\r\n");
			if (p != NULL)
			{
				memset(buf, 0, sizeof(buf));
				memcpy(buf, parseBuf, p-parseBuf);
				int arrayNum = atoi(buf);				
				if (arrayNum != 2)
				{
					m_valid = true;
//					m_logger.error("array num %d must be 2, buf %s, parseBuf %s", arrayNum, buf, parseBuf);
					LOG(ERROR)<<"array num "<<arrayNum<<" must be 2, buf "<<buf<<", parseBuf "<<parseBuf;
					return false;
				}
				else
				{
					m_parseState=REDIS_PARSE_CURSORLEN;
					parseBuf = p+2;
//					m_logger.debug("parse arrayNum 2");
					LOG(INFO)<<"parse arrayNum 2";
				}
			}
			else
			{
				goto check_buf;
			}
		}
		else if(m_parseState==REDIS_PARSE_CURSORLEN)
		{
			if (*parseBuf == '$') // TODO must be '*'
			{
				++parseBuf;
			}
			else
			{
//				m_logger.error("parse cursorlen error, %c, parseBuf %s", *parseBuf, parseBuf);
				LOG(ERROR)<<"parse cursorlen error, "<<*parseBuf<<", parseBuf "<<parseBuf;
				m_valid=true;
				return false;
			}
			p = strstr(parseBuf, "\r\n");
			if (p != NULL)
			{
				memset(buf, 0, sizeof(buf));
				memcpy(buf, parseBuf, p-parseBuf);
				int cursorlen = atoi(buf);				
				if (cursorlen <0 )
				{
//					m_logger.error("reply error, cursorlen is %d, buf %s, parseBuf %s", cursorlen, buf, parseBuf);
					LOG(ERROR)<<"reply error, cursorlen is "<<cursorlen<<", buf "<<buf<<", parseBuf "<<parseBuf;
					m_valid=true;
					return false;
				}
				else
				{
					m_parseState = REDIS_PARSE_CURSOR;
					parseBuf = p+2;
				}
			}
			else
			{
				goto check_buf;
			}
		}
		else if(m_parseState==REDIS_PARSE_CURSOR)
		{
			p = strstr(parseBuf, "\r\n");
			if (p != NULL)
			{
				memset(buf, 0, sizeof(buf));
				memcpy(buf, parseBuf, p-parseBuf);
				int cursor = atoi(buf);				
				if (cursor <0 )
				{
//					m_logger.error("reply error, cursor is %d, buf %s, parseBuf %s", cursor, buf, parseBuf);
					LOG(ERROR)<<"reply error, cursor is "<<cursor<<", buf "<<buf<<", parseBuf "<<parseBuf;
					m_valid=true;
					return false;
				}
				else
				{
					m_parseState = REDIS_PARSE_KEYSLEN;
					replyInfo.intValue=cursor; // get new cursor
//					m_logger.debug("get cursor %d", cursor);
					LOG(INFO)<<"get cursor "<<cursor;
					parseBuf = p+2;
				}
			}
			else
			{
				goto check_buf;
			}
		}
		else if(m_parseState==REDIS_PARSE_KEYSLEN)
		{
			if(*parseBuf != '*')
			{
//				m_logger.error("parse keyslen error, %c, parseBuf %s", *parseBuf, parseBuf);
				LOG(ERROR)<<"parse keyslen error, "<<*parseBuf<<", parseBuf %s"<<parseBuf;
				m_valid=true;
				return false;
			}
			parseBuf ++;
			p = strstr(parseBuf, "\r\n");
			if (p != NULL)
			{
				memset(buf, 0, sizeof(buf));
				memcpy(buf, parseBuf, p-parseBuf);
				m_arrayNum = atoi(buf);				
				if(m_arrayNum < 0)
				{
//					m_logger.error("reply error, array num is %d, buf %s, parseBuf %s", m_arrayNum, buf, parseBuf);
					LOG(ERROR)<<"reply error, array num is "<<m_arrayNum<<", buf "<<buf<<", parseBuf "<<parseBuf;
					m_valid=true;
					return false;
				}
				else if (m_arrayNum == 0)
				{
					m_valid = true;
//					m_logger.debug("empty result, ok");
					LOG(INFO)<<"empty result, ok";
					return true;
				}
				else
				{
					m_parseState = REDIS_PARSE_KEYLEN;
//					m_logger.debug("prepare to parse %d keys", m_arrayNum);
					LOG(INFO)<<"prepare to parse "<<m_arrayNum<<" keys";
					parseBuf = p+2;
				}
			}
			else
			{
				goto check_buf;
			}
		}
		else if(m_parseState==REDIS_PARSE_KEYLEN)
		{
			if (*parseBuf == '$')
			{
				++parseBuf;
			}
			else
			{
//				m_logger.error("parse keylen error, %c, parseBuf %s", *parseBuf, parseBuf);
				LOG(ERROR)<<"parse keyslen error, "<<*parseBuf<<", parseBuf %s"<<parseBuf;
				m_valid=true;
				return false;
			}
			p = strstr(parseBuf, "\r\n");
			if (p != NULL)
			{
				memset(buf, 0, sizeof(buf));
				memcpy(buf, parseBuf, p-parseBuf);
				int keylen = atoi(buf); // TODO save keylen in m_arrayLen				
				if (keylen <0 )
				{
//					m_logger.error("reply error, keylen is %d, buf %s, parseBuf %s", keylen, buf, parseBuf);
					LOG(ERROR)<<"reply error, keylen is "<<keylen<<", buf "<<buf<<", parseBuf "<<parseBuf;
					m_valid=true;
					return false;
				}
				else
				{
//					m_logger.debug("get keylen %d", keylen);
					LOG(ERROR)<<"get keylen "<<keylen;
					m_parseState = REDIS_PARSE_KEY;
					parseBuf = p+2;
				}
			}
			else
			{
				goto check_buf;
			}
		}
		else if(m_parseState==REDIS_PARSE_KEY)
		{
			p = strstr(parseBuf, "\r\n");
			if (p != NULL)
			{
				memset(buf, 0, sizeof(buf));
				memcpy(buf, parseBuf, p-parseBuf);
				int keylen=p-parseBuf; // TODO check keylen
				ReplyArrayInfo arrayInfo;
				arrayInfo.arrayLen = keylen;
				arrayInfo.arrayValue = (char*)malloc(arrayInfo.arrayLen+1);
				memset(arrayInfo.arrayValue, 0, arrayInfo.arrayLen+1);
				memcpy(arrayInfo.arrayValue, parseBuf, arrayInfo.arrayLen);
				arrayInfo.replyType = RedisReplyType::REDIS_REPLY_STRING;
				replyInfo.arrayList.push_back(arrayInfo);
				m_doneNum++; // TODO rep;yInfo.arrayList.size()
				
				if (m_doneNum < m_arrayNum)
				{
					m_parseState = REDIS_PARSE_KEYLEN;
					parseBuf += keylen+2; // parseBuf = p+2;
//					m_logger.debug("done %d, sum %d", m_doneNum, m_arrayNum);
					LOG(ERROR)<<"done "<<m_doneNum<<", sum "<<m_arrayNum;
				}
				else
				{
//					m_logger.debug("get all %d keys ok", m_arrayNum);
					LOG(INFO)<<"get all "<<m_arrayNum<<" keys ok";
					m_valid=true;
					return true;
				}
			}
			else
			{
				goto check_buf;
			}
		}
		else
		{
//			m_logger.error("unknown parse state %d", m_parseState);
			LOG(ERROR)<<"unknown parse state "<<m_parseState;
			m_valid=true;
			return false;
		}
	}

check_buf:
	if (!m_valid)
	{
		if (end-parseBuf >= 1)
		{
			m_unparseLen = end-parseBuf;
			m_unparseBuf = (char*)malloc(m_unparseLen+1);
			memset(m_unparseBuf, 0, m_unparseLen+1);
			memcpy(m_unparseBuf, parseBuf, m_unparseLen);
		}
	}
	return true;
}

bool RedisConnection::parse(char * parseBuf,int32_t parseLen,RedisReplyInfo & replyInfo)
{
//	m_logger.debug("connection:[%p] start to parse redis response:[%s], parseLen:%d.", this, parseBuf, parseLen);
	LOG(INFO)<<"connection:["<<this<<"] start to parse redis response:["<<parseBuf<<"], parseLen: "<<parseLen;
	const char * const end = parseBuf + parseLen;
	char *p = NULL;
	if (m_parseState == REDIS_PARSE_UNKNOWN_STATE && parseBuf != NULL)
	{
		m_parseState = REDIS_PARSE_TYPE;
	}
	bool haveArray = false;
	char buf[4096];
	while(parseBuf < end)
	{
		switch(m_parseState)
		{
			case REDIS_PARSE_TYPE:
				{
					switch(*parseBuf)
					{
						case '-':
							m_parseState = REDIS_PARSE_RESULT;
							replyInfo.replyType = RedisReplyType::REDIS_REPLY_ERROR;
							break;
						case '+':
							m_parseState = REDIS_PARSE_RESULT;
							replyInfo.replyType = RedisReplyType::REDIS_REPLY_STATUS;
							break;
						case ':':
							m_parseState = REDIS_PARSE_INTEGER;
							replyInfo.replyType = RedisReplyType::REDIS_REPLY_INTEGER;
							break;
						case '$':
							m_parseState = REDIS_PARSE_LENGTH;
							replyInfo.replyType = RedisReplyType::REDIS_REPLY_STRING;
							m_arrayNum = 1;
							break;
						case '*':
							m_parseState = REDIS_PARSE_ARRAYLENGTH;
							replyInfo.replyType = RedisReplyType::REDIS_REPLY_ARRAY;
							break;
						default:
							replyInfo.replyType = RedisReplyType::REDIS_REPLY_UNKNOWN;
//							m_logger.warn("recv unknown type redis response.");
							LOG(ERROR)<<"recv unknown type redis response.";
							return false;

					}
				}
				break;
			case REDIS_PARSE_INTEGER:
				{
					p = strstr(parseBuf, "\r\n");
					if (p != NULL)
					{
						memset(buf, 0, 4096);
						memcpy(buf, parseBuf, p-parseBuf);
						replyInfo.intValue = atoi(buf);
						m_valid = true;
						parseBuf = p;
						//parse '\r'
						++parseBuf;
					}
					else
					{
						goto check_buf;
					}
				}
				break;
			case REDIS_PARSE_RESULT:
				{
					p = strstr(parseBuf, "\r\n");
					if (p != NULL)
					{
						memset(buf, 0, 4096);
						memcpy(buf, parseBuf, p-parseBuf);
						replyInfo.resultString = buf;
						m_valid = true;
						parseBuf = p;
						//parse '\r'
						++parseBuf;
					}
					else
					{
						goto check_buf;
					}
				}
				break;
			case REDIS_PARSE_LENGTH:
				{
					if (haveArray && (*parseBuf == '-' || *parseBuf == '+' || *parseBuf == ':'))
					{
						p = strstr(parseBuf, "\r\n");
						if (p != NULL)
						{
							ReplyArrayInfo arrayInfo;
							arrayInfo.arrayLen = p-parseBuf;
							if (arrayInfo.arrayLen <= 0)
							{
								goto check_buf;
							}
							arrayInfo.replyType = RedisReplyType::REDIS_REPLY_STRING;
							arrayInfo.arrayValue = (char*)malloc(arrayInfo.arrayLen+1);
							//for string last char
							memset(arrayInfo.arrayValue, 0, arrayInfo.arrayLen+1);
							memcpy(arrayInfo.arrayValue, parseBuf, arrayInfo.arrayLen);
							replyInfo.arrayList.push_back(arrayInfo);
							m_doneNum++;
							if (m_doneNum < m_arrayNum)
							{
								m_parseState = REDIS_PARSE_LENGTH;
							}
							else
							{
								m_valid = true;
							}
							parseBuf = p;
							//parse '\r'
							++parseBuf;
						}
						else
						{
							goto check_buf;
						}
						break;
					}
					//for array data,may be first is $
					if (*parseBuf == '$')
					{
						++parseBuf;
					}
					p = strstr(parseBuf, "\r\n");
					if (p != NULL)
					{
						memset(buf, 0, 4096);
						memcpy(buf, parseBuf, p-parseBuf);
						m_arrayLen = atoi(buf);
						parseBuf = p;
						//parse '\r'
						++parseBuf;
						if (m_arrayLen != -1)
						{
							m_parseState = REDIS_PARSE_STRING;
						}
						else
						{
							ReplyArrayInfo arrayInfo;
							arrayInfo.arrayLen = -1;
							arrayInfo.replyType = RedisReplyType::REDIS_REPLY_NIL;
							replyInfo.arrayList.push_back(arrayInfo);
							m_doneNum++;
							if (m_doneNum < m_arrayNum)
							{
								m_parseState = REDIS_PARSE_LENGTH;
							}
							else
							{
								m_valid = true;
							}
						}
					}
					else
					{
						goto check_buf;
					}
				}
				break;
			case REDIS_PARSE_STRING:
				{
					//can not use strstr,for maybe binary data.
					//fix for if not recv \r\n,must recv \r\n.
					if (end-parseBuf >= (m_arrayLen+2))
					{
						ReplyArrayInfo arrayInfo;
						arrayInfo.arrayLen = m_arrayLen;
						arrayInfo.arrayValue = (char*)malloc(arrayInfo.arrayLen+1);
						//for string last char
						memset(arrayInfo.arrayValue, 0, arrayInfo.arrayLen+1);
						memcpy(arrayInfo.arrayValue, parseBuf, arrayInfo.arrayLen);
						arrayInfo.replyType = RedisReplyType::REDIS_REPLY_STRING;
						replyInfo.arrayList.push_back(arrayInfo);
						m_doneNum++;
						parseBuf += m_arrayLen;
						//parse '\r'
						++parseBuf;
						if (m_doneNum < m_arrayNum)
						{
							m_parseState = REDIS_PARSE_LENGTH;
						}
						else
						{
							m_valid = true;
						}
					}
					else
					{
						goto check_buf;
					}
				}
				break;
			case REDIS_PARSE_ARRAYLENGTH:
				{
					p = strstr(parseBuf, "\r\n");
					if (p != NULL)
					{
						memset(buf, 0, 4096);
						memcpy(buf, parseBuf, p-parseBuf);
						m_arrayNum = atoi(buf);
						parseBuf = p;
						//parse '\r'
						++parseBuf;
						if (m_arrayNum == 0)
						{
							m_valid = true;
						}
						else
						{
							//add for exec failed reply.
							if (m_arrayNum == -1)
							{
								m_valid = true;
								replyInfo.intValue = -1;
							}
							else
							{
								m_parseState = REDIS_PARSE_LENGTH;
								haveArray = true;
							}
						}
					}
					else
					{
						goto check_buf;
					}
				}
				break;
		}
		++parseBuf;
	}
	
check_buf:
	if (!m_valid)
	{
		if (end-parseBuf >= 1)
		{
			m_unparseLen = end-parseBuf;
			m_unparseBuf = (char*)malloc(m_unparseLen+1);
			memset(m_unparseBuf, 0, m_unparseLen+1);
			memcpy(m_unparseBuf, parseBuf, m_unparseLen);
		}
	}
	return true;
}

void RedisConnection::checkConnectionStatus()
{
	//check if the socket is closed by peer, maybe it's in CLOSE_WAIT state
	if (m_socket.fd >= 0) 
	{
		unsigned char buf[1];
		int flags = fcntl(m_socket.fd, F_GETFL, 0);
		fcntl(m_socket.fd, F_SETFL, flags | O_NONBLOCK);
		int nRead = ::read(m_socket.fd, buf, sizeof(buf));
		if (nRead == 0) 
		{
//			m_logger.debug("connection:[%p] the connection to %s:%d has been closed by peer before", this, m_socket.m_connectToHost.c_str(), m_socket.m_connectToPort);
			LOG(INFO)<<"connection:["<<this<<"] the connection to "<<m_socket.m_connectToHost<<":"<<m_socket.m_connectToPort<<" has been closed by peer before";
			m_socket.close();
		}
		fcntl(m_socket.fd, F_SETFL, flags);
	}
}

