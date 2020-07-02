#ifndef __WEBSOCKETREQUESTHANDLERFACTORY_H__
#define __WEBSOCKETREQUESTHANDLERFACTORY_H__

#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"

using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;

namespace base {

class WebsocketServer;

class WebSocketRequestHandlerFactory
	: public Poco::Net::HTTPRequestHandlerFactory
{
public:
	explicit WebSocketRequestHandlerFactory(WebsocketServer* server);

	Poco::Net::HTTPRequestHandler* 
		createRequestHandler(const HTTPServerRequest& request) override;

private:
	WebsocketServer* server_;
};

}//namespace base

#endif//__WEBSOCKETREQUESTHANDLERFACTORY_H__
