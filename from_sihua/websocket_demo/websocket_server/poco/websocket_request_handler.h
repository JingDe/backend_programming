#ifndef __WEBSOCKETREQUESTHANDLER_H__
#define __WEBSOCKETREQUESTHANDLER_H__

#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"

using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;

namespace base {

class WebsocketServer;

class WebSocketRequestHandler
	: public Poco::Net::HTTPRequestHandler
{
public:
	explicit WebSocketRequestHandler(WebsocketServer* server);

	void handleRequest(
		HTTPServerRequest&  request,
		HTTPServerResponse& response) override;

private:
	WebsocketServer* server_;
};

}//namespace base

#endif//__WEBSOCKETREQUESTHANDLER_H__
