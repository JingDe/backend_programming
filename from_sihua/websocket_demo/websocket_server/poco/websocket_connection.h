#ifndef __WEBSOCKETCONNECTION_H__
#define __WEBSOCKETCONNECTION_H__

#include <atomic>
#include <memory>
#include <thread>

#include "Poco/Net/WebSocket.h"
#include "Poco/Net/SocketStream.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/NetException.h"
#include "Poco/Thread.h"
#include "Poco/Buffer.h"

#include "websocket_server_define.h"

using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::SocketStream;
using Poco::Net::WebSocket;
using Poco::Net::WebSocketException;

namespace base {

class WebsocketConnection
{
public:
	WebsocketConnection(
		HTTPServerRequest&  request,
		HTTPServerResponse& response);

	~WebsocketConnection();

	WebSocketPtr getWebSocket()  const;

	std::string getAddress()     const;

	std::string getPeerAddress() const;

	//void shutdown();

	void close();

private:
	WebSocketPtr socket_;

	std::atomic<bool> isCloseOrInvalid_ = false;
};

}//namespace base

#endif//__WEBSOCKETCONNECTION_H__
