#ifndef __WEBSOCKET_SERVER_DEFINE_H__
#define __WEBSOCKET_SERVER_DEFINE_H__

#include <memory>

#include "Poco/Net/WebSocket.h"

using Poco::Net::WebSocket;

namespace base {

	class WebsocketConnection;

	using WebSocketPtr  = std::shared_ptr<WebSocket>;

	using WS_ConnectPtr = std::shared_ptr<WebsocketConnection>;

}//namespace base

#endif//__WEBSOCKET_SERVER_DEFINE_H__
