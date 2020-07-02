#include "base_library/log.h"

#include "websocket_connection.h"

namespace base {

WebsocketConnection::WebsocketConnection(
	HTTPServerRequest&  request,
	HTTPServerResponse& response)
{
	socket_ = std::make_shared<WebSocket>(request, response);
}

WebsocketConnection::~WebsocketConnection()
{
}

WebSocketPtr WebsocketConnection::getWebSocket() const
{
	if (false == isCloseOrInvalid_)
	{
		return socket_;
	}
	else
	{
		return nullptr;
	}
}

std::string WebsocketConnection::getAddress() const
{
	if (isCloseOrInvalid_)
	{
		return {};
	}

	try
	{
		return socket_->address().toString();
	}
	catch (Poco::Net::NetException& exc)
	{
        std::string log;
        log += "WebsocketConnection getAddress NetException: ";
        log += exc.what();
        LOG_WRITE_ERROR(log);
		return {};
	}
	catch (Poco::Exception& exc)
	{
        std::string log;
        log += "WebsocketConnection getAddress Exception: ";
        log += exc.what();
        LOG_WRITE_ERROR(log);
		return {};
	}
	catch (std::exception& exc)
	{
        std::string log;
        log += "WebsocketConnection getAddress std::exception: ";
        log += exc.what();
        LOG_WRITE_ERROR(log);
		return {};
	}
	catch (...)
	{
        LOG_WRITE_ERROR("WebsocketConnection getAddress ...");
		return {};
	}
}

std::string WebsocketConnection::getPeerAddress() const
{
	if (isCloseOrInvalid_)
	{
		return {};
	}

	try
	{
		return socket_->peerAddress().toString();
	}
	catch (Poco::Net::NetException& exc)
	{
        std::string log;
        log += "WebsocketConnection peerAddress NetException: ";
        log += exc.what();
        LOG_WRITE_ERROR(log);
		return {};
	}
	catch (Poco::Exception& exc)
	{
        std::string log;
        log += "WebsocketConnection peerAddress Exception: ";
        log += exc.what();
        LOG_WRITE_ERROR(log);
		return {};
	}
	catch (std::exception& exc)
	{
        std::string log;
        log += "WebsocketConnection peerAddress std::exception: ";
        log += exc.what();
        LOG_WRITE_ERROR(log);
		return {};
	}
	catch (...)
	{
        LOG_WRITE_ERROR("WebsocketConnection peerAddress ...");
		return {};
	}
}

//void WebsocketConnection::shutdown()
//{
//	try
//	{
//		socket_->shutdown();
//	}
//	catch (Poco::Net::NetException& exc)
//	{
//		GB_LOG(G_ERROR) << "WebsocketConnection shutdown NetException: " << exc.what() << "\n";
//	}
//	catch (Poco::Exception& exc)
//	{
//		GB_LOG(G_ERROR) << "WebsocketConnection shutdown Exception: " << exc.what() << "\n";
//	}
//	catch (std::exception& exc)
//	{
//		GB_LOG(G_ERROR) << "WebsocketConnection shutdown std::exception: " << exc.what() << "\n";
//	}
//	catch (...)
//	{
//		GB_LOG(G_ERROR) << "WebsocketConnection shutdown ...\n";
//	}
//}

void WebsocketConnection::close()
{
	if (isCloseOrInvalid_)
	{
        std::string log;
		log += "WebsocketConnection has closed already";
        LOG_WRITE_ERROR(log);
		return;
	}

	try
	{
		socket_->close();
	}
	catch (Poco::Net::NetException& exc)
	{
        std::string log;
        log += "WebsocketConnection close NetException: ";
        log += exc.what();
        LOG_WRITE_ERROR(log);
	}
	catch (Poco::Exception& exc)
	{
        std::string log;
        log += "WebsocketConnection close Exception: ";
        log += exc.what();
        LOG_WRITE_ERROR(log);
	}
	catch (std::exception& exc)
	{
        std::string log;
        log += "WebsocketConnection close std::exception: ";
        log += exc.what();
        LOG_WRITE_ERROR(log);
	}
	catch (...)
	{
        std::string log;
		log += "WebsocketConnection close ...";
        LOG_WRITE_ERROR(log);
	}
}

}//namespace base
