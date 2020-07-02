#include "websocket_server.h"
#include "websocket_connection.h"
#include "websocket_request_handler.h"

namespace base {

WebSocketRequestHandler::WebSocketRequestHandler(WebsocketServer* server)
	: server_(server)
{
}

void WebSocketRequestHandler::handleRequest(
	HTTPServerRequest&  request,
	HTTPServerResponse& response)
{
	WS_ConnectPtr ws = std::make_shared<WebsocketConnection>(request, response);

	//ws->setBlocking(false);
	server_->newConnection(ws);
}

}//namespace base
