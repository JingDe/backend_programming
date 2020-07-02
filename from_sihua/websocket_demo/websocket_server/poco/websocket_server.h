#ifndef __WEBSOCKETSERVER_H__
#define __WEBSOCKETSERVER_H__

#include <atomic>
#include <memory>
#include <thread>
#include <functional>

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

#include "Poco/ErrorHandler.h"

#include "websocket_server_define.h"

#include "base_library/log.h"

using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::SocketAddress;
using Poco::Net::SocketStream;
using Poco::Net::WebSocket;
using Poco::Net::WebSocketException;

namespace base {

using NewConnectionCallback = std::function<void(const WS_ConnectPtr&)>;

class WebsocketServer
{
	using HTTPServer       = Poco::Net::HTTPServer;
	using ServerSocket     = Poco::Net::ServerSocket;
	using HTTPServerParams = Poco::Net::HTTPServerParams;
public:
	WebsocketServer(
		const std::shared_ptr<SocketAddress>& addr,
		NewConnectionCallback newConCB,
		HTTPServerParams* params = nullptr);

	~WebsocketServer();

	void start();

	void stop();

	void addEventHandler(
		const WebSocketPtr& ws,
		const Poco::AbstractObserver& observer);

	bool hasEventHandler(
		const WebSocketPtr& ws,
		const Poco::AbstractObserver& observer);

	void removeEventHandler(
		const WebSocketPtr& ws,
		const Poco::AbstractObserver& observer);

private:
	class InnerRunnable : public Poco::Runnable
	{
	public:
		explicit InnerRunnable(WebsocketServer* server)
			: server_(server)
		{
		}
		~InnerRunnable()
		{
		}

	public:
		void run() override
		{
            LOG_WRITE_INFO("websocket_server InnerRunnable run begin");

			try
			{
				server_->server_->start();
			}
			catch (Poco::Net::NetException& exc)
			{
                std::string log;
                log += "WebsocketServer inner run start NetException: ";
                log += exc.what();
                log += ", address: ";
                log += server_->address_->toString();
                LOG_WRITE_ERROR(log);
                exit(-1);
			}
			catch (Poco::Exception& exc)
			{
                std::string log;
                log += "WebsocketServer inner run start Exception: ";
                log += exc.what();
                log += ", address: ";
                log += server_->address_->toString();
                LOG_WRITE_ERROR(log);
                exit(-1);
			}
			catch (std::exception& exc)
			{
                std::string log;
                log += "WebsocketServer inner run start std::exception: ";
                log += exc.what();
                log += ", address: ";
                log += server_->address_->toString();
                LOG_WRITE_ERROR(log);
                exit(-1);
			}
			catch (...)
			{
                std::string log;
                log += "WebsocketServer inner run start ... exception: ";
                log += "address: ";
                log += server_->address_->toString();
                LOG_WRITE_ERROR(log);
                exit(-1);
			}

			server_->reactor_.run();

            LOG_WRITE_INFO("websocket_server InnerRunnable run end");
		}

	private:
		WebsocketServer* server_;
	};

private:
	friend class WebSocketRequestHandler;
	void newConnection(const WS_ConnectPtr& ws);

private:
	std::shared_ptr<SocketAddress> address_;

	std::shared_ptr<ServerSocket> socket_;
	std::shared_ptr<HTTPServer>   server_;

	Poco::Net::SocketReactor reactor_;
	std::atomic_bool isRun_;

	NewConnectionCallback newConCB_;

	InnerRunnable runnable_;
	Poco::Thread  thread_;
};

}//namespace base

#endif//__WEBSOCKETSERVER_H__
