#ifndef CALLBACKS_H_
#define CALLBACK_H_

#include<functional>
#include<memory>

#include"TimerQueue.muduo/Timestamp.h"

class Buffer;
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;

#endif
