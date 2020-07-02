#include <thread>

#include "websocket_client.h"

namespace base {

// Resolver and socket require an io_context
WebsocketClient::WebsocketClient(
	const std::string& host,
	const unsigned int port,
	const std::string& serverPath,
	const WS_DisconnectionCallback& disconCB,
	const WS_OnReadCallback& readCB)
	: host_(host)
	, port_(port)
	, serverPath_(serverPath)
	, httpClientSession_(host, port)
	, httpRequest_(HTTPRequest::HTTP_GET, serverPath.c_str(), HTTPRequest::HTTP_1_1)
	, disconnectionCallback_(disconCB)
	, onReadCallback_(readCB)
	, isRun_(false)
	, readRunable_(this)
{
    std::stringstream beginLog;
    beginLog << "WebsocketClient::WebsocketClient() begin"
        << ", host: " << host_
        << ", port: " << port_
        << ", serverPath: " << serverPath_;
    LOG_WRITE_INFO(beginLog.str());

    LOG_WRITE_INFO("WebsocketClient::WebsocketClient() end");
}

WebsocketClient::~WebsocketClient()
{
    LOG_WRITE_INFO("WebsocketClient::~WebsocketClient() begin");

    LOG_WRITE_INFO("WebsocketClient::~WebsocketClient() end");
}

bool WebsocketClient::open()
{
	if (isRun_)
	{
		return true;
	}

	try
	{
		while (true)
		{
			try
			{
				ws_ = std::make_shared<WebSocket>(httpClientSession_, httpRequest_, response_);
                std::stringstream log;
                log << "Connect CMS"
                    << "(Ip: " << host_
                    << ", Port: " << port_
                    << ", Path: " << serverPath_
                    << ") success";
                LOG_WRITE_INFO(log.str());
				break;
			}
			catch (const Poco::Net::NetException& exc)
			{
                std::stringstream log;
                log << "Connect CMS"
                    << "(Ip: " << host_
                    << ", Port: " << port_
                    << ", Path: " << serverPath_
                    << ") Poco::Net::NetException fail!!! trying to reconnection, "
                    << "exception: " << exc.what();
                LOG_WRITE_ERROR(log.str());
				std::this_thread::sleep_for(std::chrono::seconds(3));
			}
			catch (Poco::Exception& exc)
			{
                std::stringstream log;
                log << "Connect CMS"
                    << "(Ip: " << host_
                    << ", Port: " << port_
                    << ", Path: " << serverPath_
                    << ") Poco::Exception fail!!! trying to reconnection, "
                    << "exception: " << exc.what();
                LOG_WRITE_ERROR(log.str());
				std::this_thread::sleep_for(std::chrono::seconds(3));
			}
			catch (std::exception& exc)
			{
                std::stringstream log;
                log << "Connect CMS"
                    << "(Ip: " << host_
                    << ", Port: " << port_
                    << ", Path: " << serverPath_
                    << ") std::exception fail!!! trying to reconnection, "
                    << "std::exception: " << exc.what();
                LOG_WRITE_ERROR(log.str());
				std::this_thread::sleep_for(std::chrono::seconds(3));
			}
			catch (...)
			{
                std::stringstream log;
                log << "Connect CMS"
					<< "(Ip: "    << host_
					<< ", Port: " << port_
					<< ", Path: " << serverPath_
					<< ") ... exception fail!!! trying to reconnection";
                LOG_WRITE_ERROR(log.str());
				std::this_thread::sleep_for(std::chrono::seconds(3));
			}
		}

		isRun_ = true;
		thread_.start(readRunable_);
	}
	catch (const Poco::Net::NetException& exc)
	{
		std::string msg("WebsocketClient::open() NetException: ");
		msg += exc.message();

		msg += "(Ip: ";
		msg += host_;
		msg += ", Port: ";
		msg += port_;
		msg += ", Path: ";
		msg += serverPath_;
		msg += ")";

        LOG_WRITE_ERROR(msg);
        exit(-1);

		return false;
	}
	catch (Poco::Exception& exc)
	{
		std::string msg("WebsocketClient::open() Poco::Exception: ");
		msg += exc.message();

		msg += "(Ip: ";
		msg += host_;
		msg += ", Port: ";
		msg += port_;
		msg += ", Path: ";
		msg += serverPath_;
		msg += ")";

        LOG_WRITE_ERROR(msg);
        exit(-1);

		return false;
	}
	catch (std::exception& exc)
	{
		std::string msg("WebsocketClient::open() std::exception: ");
		msg += exc.what();

		msg += "(Ip: ";
		msg += host_;
		msg += ", Port: ";
		msg += port_;
		msg += ", Path: ";
		msg += serverPath_;
		msg += ")";

        LOG_WRITE_ERROR(msg);
        exit(-1);
	}
	catch (...)
	{
		std::string msg("WebsocketClient::open() ... exception: ");

		msg += "(Ip: ";
		msg += host_;
		msg += ", Port: ";
		msg += port_;
		msg += ", Path: ";
		msg += serverPath_;
		msg += ")";

        LOG_WRITE_ERROR(msg);
        exit(-1);
	}

	return true;
}

int WebsocketClient::write(const std::string& data)
{	
	int size = -1;

	//try
	//{
		size = ws_->sendFrame(data.data(), (int)data.size());
	//}
	//catch (Poco::Net::NetException& exc)
	//{
	//	return -1;
	//}

	return size;
}

void WebsocketClient::close()
{
	//if (false == isRun_)
	//{
	//	return;
	//}

	/*
	class CInnerRunnable : public Poco::Runnable
	{
	public:
		CInnerRunnable(WebsocketClient* const client)
			: client_(client)
		{
		}
		~CInnerRunnable()
		{
		}

	public:
		void run() override
		{
			client_->isRun_ = false;
			client_->thread_.join();
			client_->ws_->shutdownReceive();			
		}

	private:
		WebsocketClient* client_;
	};

	GB_LOG(G_INFO) << "WebsocketClient::close() 1" << "\n";
	CInnerRunnable run(this);
	GB_LOG(G_INFO) << "WebsocketClient::close() 2" << "\n";
	Poco::Thread thread;
	GB_LOG(G_INFO) << "WebsocketClient::close() 3" << "\n";
	thread.start(run);
	GB_LOG(G_INFO) << "WebsocketClient::close() 4" << "\n";
	thread.join();
	GB_LOG(G_INFO) << "WebsocketClient::close() 5" << "\n";*/

	//GB_LOG(G_INFO) << "Tag=" << tag() << ", WebsocketClient::close() begin" << "\n";
    	
    try
    {
        LOG_WRITE_ERROR("WebsocketClient::close() 1");
        isRun_ = false;
        ws_->shutdownReceive();
        ws_->close();
        LOG_WRITE_ERROR("WebsocketClient::close() 2");
        thread_.join();
        //ws_->shutdownSend();
        LOG_WRITE_ERROR("WebsocketClient::close() 3");
    }
    catch (...)
    {
        std::stringstream log;
        log << "WebsocketClient::close() error";
        LOG_WRITE_ERROR(log.str());
    }

	//GB_LOG(G_INFO) << "Tag=" << tag() << ", WebsocketClient::close() end" << "\n";
}

//void WebsocketClient::close()
//{
//	if (false == isRun_)
//	{
//		return;
//	}
//	else
//	{
//		isRun_ = false;
//		//ws_->shutdown();
//		ws_->close();
//
//		std::string msg("close...");
//		disconnectionCallback_(msg);
//	}
//}

}//namespace base
