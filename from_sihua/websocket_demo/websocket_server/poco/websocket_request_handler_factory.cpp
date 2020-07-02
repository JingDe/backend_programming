#include "websocket_request_handler.h"
#include "websocket_request_handler_factory.h"

namespace base {

WebSocketRequestHandlerFactory::WebSocketRequestHandlerFactory(WebsocketServer* server)
	: server_(server)
{
}

Poco::Net::HTTPRequestHandler* WebSocketRequestHandlerFactory::createRequestHandler(
	const HTTPServerRequest& request)
{
	return new WebSocketRequestHandler(server_);
}

}//namespace base
