#ifndef SOCKET_H_
#define SOCKET_H_


struct tcp_info;
class InetAddress;

class Socket {
public:
	Socket(int fd) :sockfd_(fd) {}
	~Socket();

	int fd() const { return sockfd_; }
	bool getTcpInfo(struct tcp_info*) const;
	bool getTcpInfoString(char* buf, int len) const;

	void bindAddress(const InetAddress& localaddr);
	void listen();
	int accept(InetAddress* peeraddr);

	void shutdownWrite();

	void setTcpNoDelay(bool on);
	void setReuseAddr(bool on);
	void setReusePort(bool on);
	void setKeepAlive(bool on);

private:
	int sockfd_;
};

#endif 