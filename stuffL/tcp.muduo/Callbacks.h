#ifndef CALLBACKS_H_
#define CALLBACK_H_

#include<functional>
#include<memory>

#include"TimerQueue.muduo/Timestamp.h"

class Buffer;
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr&, const char*, ssize_t)> MessageCallback; // s05
typedef std::function<void(const TcpConnectionPtr&)> CloseCallback; // s06

#endif
