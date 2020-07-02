#ifndef __WEBSOCKETCLIENT_H__
#define __WEBSOCKETCLIENT_H__

#include <atomic>
#include <functional>

#include <sstream>

#include "base_library/log.h"

#include "Poco/Net/WebSocket.h"
#include "Poco/Net/SocketStream.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/NetException.h"
#include "Poco/Thread.h"
#include "Poco/Buffer.h"

using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::SocketStream;
using Poco::Net::WebSocket;
using Poco::Net::WebSocketException;

namespace base {

using WS_DisconnectionCallback = std::function<void(std::string&)>;
using WS_OnReadCallback = std::function<void(const std::string&)>;

class WebsocketClient
{
public:
	// Resolver and socket require an io_context
	WebsocketClient(
		const std::string& host,
		const unsigned int port,
		const std::string& serverPath,
		const WS_DisconnectionCallback& disconCB,
		const WS_OnReadCallback& readCB);

	~WebsocketClient();

	bool open();

	int write(const std::string& data);

	void close();

private:
	class InnerRunnable : public Poco::Runnable
	{
	public:
		InnerRunnable(WebsocketClient* const client)
			: client_(client)
		{
			auto server{ client->host_ + ":" + std::to_string(client->port_) };
            std::stringstream beginLog;
            beginLog << "WebsocketClient::InnerRunnable begin server=" << server;
            LOG_WRITE_INFO(beginLog.str());

            std::stringstream endLog;
            endLog << "WebsocketClient::InnerRunnable end server=" << server;
            LOG_WRITE_INFO(endLog.str());
		}

		~InnerRunnable()
		{
			auto server{ client_->host_ + ":" + std::to_string(client_->port_) };
            std::stringstream beginLog;
            beginLog << "WebsocketClient::~InnerRunnable begin server=" << server;
            LOG_WRITE_INFO(beginLog.str());

            std::stringstream endLog;
            endLog << "WebsocketClient::~InnerRunnable end server=" << server;
            LOG_WRITE_INFO(endLog.str());
		}

	public:
		void run() override
		{
			while (client_->isRun_)
			{
				auto exception_lambda 
					= [this](const std::string& msg, const std::string& exceptionMsg)
				{
					std::string strLog = msg;
					strLog += exceptionMsg;
					client_->isRun_ = false;
					client_->disconnectionCallback_(strLog);
                    std::stringstream log;
                    log << "Websocket client receiveFrame exception: " << exceptionMsg;
                    LOG_WRITE_ERROR(log.str());
				};

				try
				{
					client_->ws_->setReceiveTimeout(Poco::Timespan(3, 0, 0, 0, 0));

					struct ReceiveBuffer
					{
						ReceiveBuffer(int bufsize) 
							: size(bufsize) 
						{ buf = (char*)malloc(bufsize); }
						~ReceiveBuffer() { if (buf) { free(buf); } }

						char* buf  = nullptr;
						int   size = 0;
					};

					//char buffer[1*1024*1024] = { 0 };//
					ReceiveBuffer buffer(32 * 1024 * 1024);
					int flags;
					const int size = client_->ws_->receiveFrame(buffer.buf, buffer.size, flags);
					if (size == buffer.size)
					{
						//Warning
                        std::stringstream log;
						log << "websocket client receiveFrame buffer is full!!! buffer size: " << size;
                        LOG_WRITE_WARNING(log.str());
					}

					if (size <= 0)
					{
                        std::stringstream log;
                        log << "websocket client receiveFrame size<=0: " << size;
                        LOG_WRITE_ERROR(log.str());
						std::string data{ "websocket client receiveFrame size<=0:" };
						client_->disconnectionCallback_(data);
						return;
					}

					std::string msg(buffer.buf, size);
					//GB_LOG(G_INFO) << "Client Tag=" << client_->tag() 
					//	<< ", websocket client receiveFrame size=" << size << "\n";
					client_->onReadCallback_(msg);
				}				
				catch (const Poco::Net::NetException& exc)
				{
					std::string msg("WebsocketClient receiveFrame NetException: ");
					exception_lambda(msg, exc.message());
					return;
				}
				catch (const Poco::IOException& exc)
				{
					std::string msg("WebsocketClient receiveFrame IOException: ");
					exception_lambda(msg, exc.message());
					return;
				}
				catch (const Poco::Exception& exc)
				{
					std::string msg("WebsocketClient receiveFrame Exception: ");
					exception_lambda(msg, exc.message());
					return;
				}
				catch (const std::exception& exc)
				{
					std::string msg("WebsocketClient receiveFrame std exception: ");
					exception_lambda(msg, exc.what());
					return;
				}
				catch (...)
				{
					std::string msg("WebsocketClient receiveFrame Exception ...");
					exception_lambda(msg, "");
					return;
				}
			}
		}

	private:
		WebsocketClient* client_;
	};
	
private:
	const std::string  host_;
	const unsigned int port_;
	const std::string  serverPath_;

	HTTPClientSession httpClientSession_;
	HTTPRequest httpRequest_;
	HTTPResponse response_;
	std::shared_ptr<WebSocket> ws_;

	WS_DisconnectionCallback disconnectionCallback_;
	WS_OnReadCallback onReadCallback_;

	std::atomic_bool isRun_;
	Poco::Thread  thread_;
	InnerRunnable readRunable_;
};

}//namespace base

#endif//__WEBSOCKETCLIENT_H__
