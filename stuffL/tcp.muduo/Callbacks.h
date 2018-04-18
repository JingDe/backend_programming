#ifndef CALLBACKS_H_
#define CALLBACK_H_

#include<functional>
#include<memory>

#include"TimerQueue.muduo/Timestamp.h"

class Buffer;
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
//typedef std::function<void(const TcpConnectionPtr&, const char*, ssize_t)> MessageCallback; // s05
typedef std::function<void(const TcpConnectionPtr&)> CloseCallback; // s06

typedef std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback; // s07

typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void(const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

#endif
