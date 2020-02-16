#include <assert.h>
#include <sys/types.h>
#include "socket.h"
#include"glog/logging.h"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include<sys/syscall.h>
#include<sys/epoll.h>

#define gettid() syscall(__NR_gettid)

Socket::Socket() :
	fd(INVALID_SOCKET_HANDLE),m_timeout(-1), localport(-1), port(0)//,  m_logger("clib.socket") 
{
	m_stream = true;
}

Socket::~Socket() {
	close();
}

int Socket::accept(Socket& s) {

	// use poll to set timeout
	if (m_timeout > 0) {
		int retval;
		struct pollfd pfd;
		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;

		retval = ::poll( &pfd, 1, m_timeout*1000 );  //milliseconds
		if( retval < 0 ) {
//			m_logger.error( "poll error, fd %d at accept, errno %d", fd, errno );
			PLOG(ERROR)<<"poll error, fd "<<fd<<" at accept";
			return -1;
		}
		if (retval == 0) {
//			m_logger.error("poll on fd(%d) at accept return timeout(%d s)", fd, m_timeout);
			LOG(ERROR)<<"poll on fd("<<fd<<") at accept return timeout("<<m_timeout<<" s)";
			return -1;
		}
	}

	if(address.getAddrType() == AF_INET){
		sockaddr_in client_addr;
		socklen_t client_len;
		client_len = sizeof(client_addr);
		int fdClient = ::accept(this->fd, (sockaddr *)&client_addr, &client_len);
		if (fdClient < 0) {
//			m_logger.error("socket accept failed: %m");
			PLOG(ERROR)<<"socket accept failed";
            return -1;
//			throw OWException("Socket", "accept", "socket accept failed");
		}

		s.address.setInternAddressV4(client_addr.sin_addr);
		s.address.setAddrType(AF_INET);
		s.fd = fdClient;
		s.port = ntohs(client_addr.sin_port);

	}else if(address.getAddrType() == AF_INET6){
		sockaddr_in6 client_addr;
		socklen_t client_len;
		client_len = sizeof(client_addr);
		int fdClient = ::accept(this->fd, (sockaddr *)&client_addr, &client_len);

		if (fdClient < 0) {
//			m_logger.error("socket accept failed: %m");
			PLOG(ERROR)<<"socket accept failed";
            return -1;
//			throw OWException("Socket", "accept", "socket accept failed");
		}
		s.address.setInternAddressV6(client_addr.sin6_addr);
		s.address.setAddrType(AF_INET6);
		s.fd = fdClient;
		s.port = ntohs(client_addr.sin6_port);

	}else{
//		m_logger.error("listen socket invalid ");
		LOG(ERROR)<<"listen socket invalid ";
	}

	s.listenIp = this->listenIp;
	s.listenPort = this->listenPort;
    return 0;
}

int Socket::__bindv4(InetAddr host, int port){
	int server_len = 0;
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_len = sizeof(sockaddr_in);
	server_addr.sin_addr = host.address.sin_addr;
	if (::bind(fd, (sockaddr *)&server_addr, server_len)== -1) {
//		m_logger.error("socket bind to %s:%d failed: %m",host.getHostAddress().c_str(), port);
		PLOG(ERROR)<<"socket bind to "<<host.getHostAddress()<<":"<<port<<" failed";
        return -1;
//		throw OWException("socket bind failed");
	}
    return 0;
}

int Socket::__bindv6(InetAddr host, int port){
	int server_len = 0;
	struct sockaddr_in6 server_addr;
	server_addr.sin6_family = AF_INET6;
	server_addr.sin6_port = htons(port);
	server_len = sizeof(struct sockaddr_in6);
	server_addr.sin6_addr = host.address.sin6_addr;

	if (::bind(fd, (sockaddr *)&server_addr, server_len)== -1) {
//		m_logger.error("socket bind to %s:%d failed: %m",host.getHostAddress().c_str(), port);
		LOG(ERROR)<<"socket bind to "<<host.getHostAddress()<<":"<<port<<" failed: %m";
        return -1;
//		throw OWException("socket bind failed");
	}
    return 0;
}

int Socket::bind(InetAddr host, int port) {

	int reuseflag = 1;
	int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuseflag,   sizeof(reuseflag));
	if (ret < 0) {
//		m_logger.error("failed to setsockopt so_reuseaddr: %m");
		PLOG(ERROR)<<"failed to setsockopt so_reuseaddr";
        return -1;
	}

	if(host.getAddrType() == AF_INET){
		ret=__bindv4(host, port);
	}else if(host.getAddrType() == AF_INET6){
		ret=__bindv6(host, port);
	}
	this->localport = port;
    return ret;
}

/** Bind this socket to the specified port on the named host. */
int Socket::bind(const string& host, int port) {
	int ret=bind(InetAddr::getByName(host), port);
	listenIp = host;
	listenPort = port;
    return ret;
}

int Socket::close() {
	if (fd != INVALID_SOCKET_HANDLE) {
//		m_logger.debug("closing socket in thread(%d), socketFd=%d", gettid(), fd);
		LOG(INFO)<<"closing socket in thread("<<gettid()<<"), socketFd="<<fd;
		if (::close(fd)== -1) {
//			m_logger.error("close socket failed: %m");
			PLOG(ERROR)<<"close socket failed";
            return -1;
//			throw OWException("close socket failed");
		}
		fd = INVALID_SOCKET_HANDLE;
		port = 0;
		localport = -1;
	}
    return 0;
}

bool Socket::connect(const string& host, int port)
{
	return connectNonBock(host, port, 0, 900000);
}

bool Socket::connectNonBock(const string& host, int port, int timeOut)
{
	return connectNonBock(host, port, timeOut, 0);
}

bool Socket::connectNonBock(const string& host, int port, int tv_sec, int tv_usec)
{
	m_connectToHost = host;
	m_connectToPort = port;
	InetAddr address=InetAddr::getByName(host);
	if (fd == INVALID_SOCKET_HANDLE) {
		create(m_stream, address.getAddrType());
	}
	if (fd == 0)
	{
//		m_logger.warn("meet fd = 0, need create socket again.");
		LOG(ERROR)<<"meet fd = 0, need create socket again.";
		create(m_stream, address.getAddrType());
	}

	if(address.getAddrType() == AF_INET){
		__connectNonBockv4(address,port,tv_sec, tv_usec);
	}else if(address.getAddrType() == AF_INET6){
		__connectNonBockv6(address,port,tv_sec, tv_usec);
	}
	this->address = address;
	this->port = port;
	return true;
}

bool Socket::__connectNonBockv4(InetAddr address, int port, int tv_sec, int tv_usec){
 	sockaddr_in client_addr;
	int client_len = sizeof(client_addr);
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr = address.address.sin_addr;
	client_addr.sin_port = htons(port);

	int flags,n,error;
	struct timeval ;
	flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	error=0;

	if( ( n = ::connect(fd,(struct sockaddr *)&client_addr, client_len ))<0){
		if( errno != EINPROGRESS ){
//			m_logger.error("connect to %s:%d failed: %m",address.getHostAddress().c_str(), port);
			PLOG(ERROR)<<"connect to "<<address.getHostAddress()<<":"<<port<<" failed";
			return false;
		}
	}

	if( n != 0){
		int retval;
		struct pollfd pfd;
		pfd.fd = fd;
		pfd.events = POLLWRNORM | POLLRDNORM;
		pfd.revents = 0;

		retval = ::poll( &pfd, 1, tv_sec*1000+tv_usec/1000 );  //milliseconds
		if( retval < 0 ) {
//			m_logger.error( "poll error at connect, fd %d, errno %d", fd, errno );
			PLOG(ERROR)<<"poll error at connect, fd "<<fd<<", errno "<<errno;
			close(); //avoid close_wait, see http://www.chinaitpower.com/A200507/2005-07-27/174296.html
			return false;
		}
		if (retval == 0) {
//			m_logger.error("poll on fd(%d) at connect return timeout(%d ms)", fd, tv_sec*1000+tv_usec/1000);
			LOG(ERROR)<<"poll on fd("<<fd<<") at connect return timeout("<<tv_sec*1000+tv_usec/1000<<" ms)";
			close(); //avoid to recv peer response when the socket is used to next command
			return false;
		}
		socklen_t len=sizeof(error);
		if(getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&len)<0){
//			m_logger.error("connect to %s:%d failed: getsockopt error:",address.getHostAddress().c_str(), port);
			PLOG(ERROR)<<"connect to "<<address.getHostAddress()<<":"<<port<<" failed: getsockopt error";
			return false;
		}
		if(error){
//			m_logger.error("connect to %s:%d failed: getsockopt.error[%d]:",address.getHostAddress().c_str(), port,error);
			LOG(ERROR)<<"connect to "<<address.getHostAddress()<<":"<<port<<" failed: getsockopt.error["<<error<<"]";
			close();
			return false;
		}
	}
	fcntl(fd, F_SETFL, flags);
//	m_logger.debug("connected to %s:%d, socketFd=%d", address.getHostAddress().c_str(), port, fd);
	LOG(INFO)<<"connected to "<<address.getHostAddress()<<":"<<port<<", socketFd="<<fd;
	return true;
}

bool Socket::__connectNonBockv6(InetAddr address, int port, int tv_sec, int tv_usec){
		sockaddr_in6 client_addr;
		int client_len = sizeof(sockaddr_in6);
		bzero(&client_addr, client_len);
		client_addr.sin6_family = AF_INET6;
		client_addr.sin6_addr = address.address.sin6_addr;
		client_addr.sin6_port = htons(port);

		int flags,n,error;
		flags = fcntl(fd, F_GETFL, 0);
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
		error=0;

		if( ( n = ::connect(fd,(struct sockaddr *)&client_addr, client_len ))<0){
			if( errno != EINPROGRESS ){
//				m_logger.error("connect to %s:%d failed: %s",address.getHostAddress().c_str(), port, strerror(errno));
				PLOG(ERROR)<<"connect to "<<address.getHostAddress()<<":"<<port<<" failed";
				return false;
			}
		}

		if( n != 0){
			int retval;
			struct pollfd pfd;
			pfd.fd = fd;
			pfd.events = POLLWRNORM | POLLRDNORM;
			pfd.revents = 0;

			retval = ::poll( &pfd, 1, tv_sec*1000+tv_usec/1000 );  //milliseconds
			if( retval < 0 ) {
//				m_logger.error( "poll error at connect, fd %d, error %s", fd, strerror(errno));
				PLOG(ERROR)<<"poll error at connect, fd "<<fd;
				close(); //avoid close_wait, see http://www.chinaitpower.com/A200507/2005-07-27/174296.html
				return false;
			}
			if (retval == 0) {
//				m_logger.error("poll on fd(%d) at connect return timeout(%d ms)", fd, tv_sec*1000+tv_usec/1000);
				LOG(ERROR)<<"poll on fd("<<fd<<") at connect return timeout("<<tv_sec*1000+tv_usec/1000<<" ms)";
				close(); //avoid to recv peer response when the socket is used to next command
				return false;
			}
			socklen_t len=sizeof(error);
			if(getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&len)<0){
//				m_logger.error("connect to %s:%d failed: getsockopt error:%s",address.getHostAddress().c_str(), port, strerror(errno));
				PLOG(ERROR)<<"connect to "<<address.getHostAddress()<<":"<<port<<" failed: getsockopt error";
				return false;
			}
			if(error){
//				m_logger.error("connect to %s:%d failed: getsockopt.error[%s]:",address.getHostAddress().c_str(), port,strerror(errno));
				PLOG(ERROR)<<"connect to "<<address.getHostAddress()<<":"<<port<<" failed: getsockopt.error";
				close();
				return false;
			}
		}

		fcntl(fd, F_SETFL, flags);
//		m_logger.debug("connected to %s:%d, socketFd=%d", address.getHostAddress().c_str(), port, fd);
		LOG(INFO)<<"connected to "<<address.getHostAddress()<<":"<<port<<", socketFd="<<fd;
		return true;
}



/**  Connects this socket to the specified port number
 on the specified host.
 */
bool Socket::__connectv4(InetAddr address, int port){
	sockaddr_in client_addr;
	int client_len = sizeof(client_addr);
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr = address.address.sin_addr;
	client_addr.sin_port = htons(port);

	if (::connect(fd, (sockaddr *)&client_addr, client_len)== -1) {
		close();
//		m_logger.error("connect to %s:%d failed: %m",address.getHostAddress().c_str(), port);
		PLOG(ERROR)<<"connect to "<<address.getHostAddress()<<":"<<port<<" failed";
		return false;
	}
//	m_logger.debug("connected to %s:%d, socketFd=%d", address.getHostAddress().c_str(), port, fd);
	LOG(INFO)<<"connect to "<<address.getHostAddress()<<":"<<port<<", socketFd="<<fd;
	this->address = address;
	this->port = port;
	return true;
}

bool Socket::__connectv6(InetAddr address, int port){
	sockaddr_in6 client_addr;
	int client_len = sizeof(client_addr);
	client_addr.sin6_family = AF_INET6;
	client_addr.sin6_addr = address.address.sin6_addr;
	client_addr.sin6_port = htons(port);

	if (::connect(fd, (sockaddr *)&client_addr, client_len)== -1) {
		close();
//		m_logger.error("connect to %s:%d failed: %m",address.getHostAddress().c_str(), port);
		PLOG(ERROR)<<"connect to "<<address.getHostAddress()<<":"<<port<<" failed";
		return false;
	}
//	m_logger.debug("connected to %s:%d, socketFd=%d", address.getHostAddress().c_str(), port, fd);
	LOG(INFO)<<"connected to "<<address.getHostAddress()<<":"<<port<<", socketFd="<<fd;
	this->address = address;
	this->port = port;
	return true;
}

bool Socket::connect(InetAddr address, int port) {
	if (fd == INVALID_SOCKET_HANDLE) {
		create(m_stream, address.getAddrType());
	}

	if(address.getAddrType() == AF_INET){
		return __connectv4(address, port);
	}else if(address.getAddrType() == AF_INET6){
		return __connectv6(address, port);
	}
	return true;
}

/** Connects this socket to the specified port on the named host. */
bool Socket::connectOriginally(const string& host, int port) {
	m_connectToHost = host;
	m_connectToPort = port;
	return connect(InetAddr::getByName(host), port);
}

/** Creates either a stream or a datagram socket. */
void Socket::create(bool stream) {
	m_stream = stream;
	create(true, AF_INET);
}

int Socket::create(bool stream, int addrType) {
	m_stream = stream;
	address.setAddrType(addrType);
	if(addrType == AF_INET){
		if ((fd = ::socket(AF_INET, m_stream ? SOCK_STREAM : SOCK_DGRAM, 0)) == -1) {
//			m_logger.error("socket create failed: %s", strerror(errno));
			PLOG(ERROR)<<"socket create failed";
            return -1;
//			throw OWException("socket create failed");
		}
	}else if(addrType == AF_INET6){
		if ((fd = ::socket(AF_INET6, m_stream ? SOCK_STREAM : SOCK_DGRAM, 0)) == -1) {
//			m_logger.error("socket create failed: %s",strerror(errno));
			PLOG(ERROR)<<"socket create failed";
            return -1;
//			throw OWException("socket create failed");
		}
	}
    return 0;
}
/** Sets the maximum queue length for incoming connection
 indications (a request to connect) to the count argument.
*/
int Socket::listen(int backlog) {
	if (::listen(fd, backlog)== -1) {
//		m_logger.error("failed to listen on port %d:%m", port); 
		PLOG(ERROR)<<"failed to listen on port "<<port;
        return -1;
//		throw OWException("socket listen failed");
	}
    return 0;
}

/*
 *  Returns the address and port of this socket as a String.
 */
string Socket::toString() const {
	char strPort[256];
	sprintf(strPort, "%d", port);
	string ret = address.getHostAddress() + ":" + strPort;
	return ret;
}

int Socket::read2(void * buf, size_t maxlen, int timeout)
{
	int len_read = 0;
	unsigned char * p = (unsigned char *)buf;

	if (timeout > 0) {
		struct timeval tv;	
		fd_set readFds;
		int ret;
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		FD_ZERO(&readFds);
		FD_SET(fd, &readFds);
		ret = ::select(fd + 1, &readFds, NULL, NULL, &tv);
		if (ret == -1) {
//			m_logger.error("select on fd(%d) return error: %m", fd);
			PLOG(ERROR)<<"select on fd("<<fd<<") return error";
//			m_logger.error("fd=%d, tv_sec=%d, tv_usec=%d", fd, tv.tv_sec, tv.tv_usec);
			LOG(ERROR)<<"fd="<<fd<<", tv_sec="<<tv.tv_sec<<", tv_usec=%d"<<tv.tv_usec;
			close(); //avoid close_wait, see http://www.chinaitpower.com/A200507/2005-07-27/174296.html
			return -1;
		} else if (ret == 0) {
			return 0;
		}
	}
		
	while ( (len_read = ::read(fd, p, maxlen)) < 0 ) {
		if (errno == EINTR) {
//			m_logger.info("interrupted when reading socket, but no problem, i'll read again.");	
			LOG(INFO)<<"interrupted when reading socket";
			continue;
		} else {
//			m_logger.error("socket read return %d : %m", len_read); 
			PLOG(ERROR)<<"socket read return "<<len_read;
			close(); //avoid close_wait, see http://www.chinaitpower.com/A200507/2005-07-27/174296.html
			return -1;
		}
	}

	if (len_read == 0) {
//		m_logger.error("connection closed by peer when reading socket, socketFd=%d", fd); 
		LOG(ERROR)<<"connection closed by peer when reading socket, socketFd="<<fd;
		close();
	}
	
	return len_read;
}

bool Socket::WatchReadEvent(int& epollfd)
{
	int epfd=epoll_create(1);
	if(epfd<0) 
	{
		PLOG(ERROR)<<"epoll_create failed";
		return false;
	}

	struct epoll_event ev;
	ev.events=EPOLLIN;
	ev.data.fd=fd;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev)<0)
	{
		PLOG(ERROR)<<"epoll_ctl ADD failed";
		::close(epfd);
		return -1;
	}

	epollfd=epfd;
	return true;	
}

bool Socket::WaitReadEvent(int epollfd)
{
	struct epoll_event event;
	int nEvents=epoll_wait(epollfd, &event, 1, -1);
	if(nEvents<0)
	{
		PLOG(ERROR)<<"epoll_wait failed";
		return false;
	}
	if(nEvents!=1)
	{
		if(errno==EINTR)
			LOG(INFO)<<"epoll_wait interrupted";
		else
			PLOG(INFO)<<"unknown error";
		return false;
	}

	LOG(INFO)<<"event.events is "<<event.events;
	// TODO EPOLLERR
	if(event.events  &  (EPOLLIN | EPOLLPRI | EPOLLRDNORM | POLLRDBAND | EPOLLRDHUP))
	{
		return true;
	}
	else
	{		
		return false;
	}
}

bool Socket::UnWatchEvent(int handler)
{
	::close(handler);
	return true;
}

int Socket::read(void * buf, size_t maxlen, int timeout) {
	int len_read = 0;
	unsigned char * p = (unsigned char *)buf;

	// use poll to implement timeout
	if (timeout > 0) {
		int retval;
		struct pollfd pfd;
		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;

		retval = ::poll( &pfd, 1, timeout*1000 );  //milliseconds
		if( retval < 0 ) {
//			if( errno == EINTR ){
//				continue;
//			}
//			m_logger.error( "poll error, fd %d, errno %d", fd, errno );
			LOG(ERROR)<<"poll error, fd "<<fd<<", errno "<<errno;
			close(); //avoid close_wait, see http://www.chinaitpower.com/A200507/2005-07-27/174296.html
			return -1;
		}
		if( (pfd.revents & POLLERR) || (pfd.revents & POLLNVAL) || (pfd.revents & POLLHUP) ) {
//			m_logger.error( "poll revents error ?? fd %d, pfd.revents %d", fd, (int)pfd.revents );
			LOG(ERROR)<<"poll revents error ?? fd "<<fd<<", pfd.revents "<<pfd.revents;
			close(); //avoid close_wait, see http://www.chinaitpower.com/A200507/2005-07-27/174296.html
			return -1;
		}
		if (retval == 0) {
//			m_logger.error("poll on fd(%d) at read return timeout(%d s)", fd, timeout);
			LOG(ERROR)<<"poll on fd("<<fd<<") at read return timeout("<<timeout<<" s)";
			close(); //avoid to recv peer response when the socket is used to next command
			return -1;
		}
	}
	
	while ( (len_read = ::read(fd, p, maxlen)) < 0 ) {
		if (errno == EINTR) {
//			m_logger.info("interrupted when reading socket, but no problem, i'll read again.");	
			LOG(INFO)<<"interrupted when reading socket";
			continue;
		} else {
//			m_logger.error("socket read return %d : %m", len_read); 
			PLOG(ERROR)<<"socket read return "<<len_read;
			close(); //avoid close_wait, see http://www.chinaitpower.com/A200507/2005-07-27/174296.html
			return -1;
		}
	}

	//avoid close_wait, see http://www.chinaitpower.com/A200507/2005-07-27/174296.html
	if (len_read == 0) {
//		m_logger.error("connection closed by peer when reading socket, socketFd=%d", fd); 
		LOG(ERROR)<<"connection closed by peer when reading socket, socketFd="<<fd;
		close();
	}
	
	return len_read;
}


int Socket::readFull(void * buf, size_t len) {
	int len_read = 0;
	unsigned char * p = (unsigned char *)buf;

	while ((size_t)(p - (unsigned char *)buf) < len) {
		//len_read = ::read(fd, p, len - (p - (unsigned char *)buf));
		len_read = read(p, len - (p - (unsigned char *)buf));
		if (len_read < 0) {
//			m_logger.error("socket read failed on port %d: %m", port); 			
			PLOG(ERROR)<<"socket read failed on port "<<port;
            return -1;
//			throw OWException("socket read failed");
		}
		if (len_read == 0) {
			break;
		}
		p += len_read;
	}

	return (p - (const unsigned char *)buf);
}

size_t Socket::writeFull(const void * buf, size_t len) {
	if (fd == INVALID_SOCKET_HANDLE) {
//		m_logger.info("can not write to socketFd which is 0");
		LOG(INFO)<<"can not write to socketFd which is 0";
		return 0;
	}

	int len_written = 0;
	const unsigned char * p = (const unsigned char *)buf;
	if(m_stream){
		while ((size_t)(p - (const unsigned char *)buf) < len) {
			len_written = ::write(fd, p, len - (p - (const unsigned char *)buf));

			if (len_written < 0) {
				if (errno == EINTR) { //interrupt
//					m_logger.info("interrupted when writing socket, but no problem, i'll continue.");	
					LOG(INFO)<<"interrupted when writing socket";
					continue;
				} else {
					//
					close();
					fd = INVALID_SOCKET_HANDLE;
					break;
				}
			}
			if (len_written == 0) {
				break;
			}
			p += len_written;
		}

		return (p - (const unsigned char *)buf);
	}else{
		struct sockaddr_in server_addr;
		memset(&server_addr,0,sizeof(server_addr));  
		server_addr.sin_family = AF_INET;  
		server_addr.sin_port = htons(m_connectToPort);  
		server_addr.sin_addr.s_addr = inet_addr(m_connectToHost.c_str());
		while(true){
			len_written = sendto(fd,buf,len,0,(struct sockaddr *)&server_addr,sizeof(server_addr));
			if (len_written < 0) {
				if (errno == EINTR) { //interrupt
//					m_logger.info("interrupted when writing socket, but no problem, i'll continue.");
					LOG(INFO)<<"interrupted when writing socket";
					continue;
				} else {
//					m_logger.error("socket write to server %s:%d failed . I'll close the socket.",m_connectToHost.c_str(), m_connectToPort);
					LOG(ERROR)<<"socket write to server "<<m_connectToHost<<":"<<m_connectToPort<<" failed. close the socket.";
					close();
					break;
				}
			}else{
                 break;
            }
                        
		}  
        return len_written;
	}
}

/** Retrive setting for SO_TIMEOUT.
 */
int Socket::getSoTimeout() const {
	return m_timeout;
}

/** Enable/disable SO_TIMEOUT with the specified timeout, in milliseconds.
 */
void Socket::setSoTimeout(int timeout) {
	m_timeout = timeout;
}

