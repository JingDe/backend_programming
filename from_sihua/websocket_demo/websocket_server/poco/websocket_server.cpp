#include "base_library/log.h"

#include "websocket_request_handler.h"
#include "websocket_request_handler_factory.h"

#include "websocket_server.h"

#include <sstream>

namespace base {

WebsocketServer::WebsocketServer(
	const std::shared_ptr<SocketAddress>& addr
	, NewConnectionCallback newConCB
	, HTTPServerParams* params)
	: address_ (addr)
	, newConCB_(newConCB)
	, isRun_   (false)
	, runnable_(this)
	//, reactor_(Poco::Timespan(100))
{
	if (nullptr == params)
	{
		params = new HTTPServerParams;
	}

	socket_ = std::make_shared<ServerSocket>(*addr.get());
	server_ = std::make_shared<HTTPServer>(
		new WebSocketRequestHandlerFactory(this), *socket_.get(), params);
}

WebsocketServer::~WebsocketServer()
{
}

void WebsocketServer::start()
{
	if (false == isRun_)
	{
		//thread_.start(runnable_);
		server_->start();
		thread_.start(reactor_);
	}
	else
	{
        LOG_WRITE_WARNING("Websocket Server has started already!!!");
	}
}

void WebsocketServer::stop()
{
	//if (true == isRun_)
	{
		isRun_ = false;
        try
        {
            if (server_ != nullptr)
            {
                server_->stop();
            }
            reactor_.stop();
        }
        catch (...)
        {
            std::stringstream log;
            log << "WebsocketClient::stop() error";
            LOG_WRITE_ERROR(log.str());
        }
        thread_.join();
	}
	//else
	//{
    //    LOG_WRITE_WARNING("Websocket Server has stopped already!!!");
	//}
}

void WebsocketServer::addEventHandler(
	const WebSocketPtr& ws,
	const Poco::AbstractObserver& observer)
{
	reactor_.addEventHandler(*ws.get(), observer);
}

bool WebsocketServer::hasEventHandler(
	const WebSocketPtr& ws,
	const Poco::AbstractObserver& observer)
{
	return reactor_.hasEventHandler(*ws.get(), observer);
}

void WebsocketServer::removeEventHandler(
	const WebSocketPtr& ws,
	const Poco::AbstractObserver& observer)
{
	reactor_.removeEventHandler(*ws.get(), observer);
}

void WebsocketServer::newConnection(const WS_ConnectPtr& ws)
{
	newConCB_(ws);
}

}//namespace base
