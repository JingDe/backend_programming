#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>
 
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
 
#include<event.h>
#include<event2/bufferevent.h>
#include<event2/buffer.h>
#include<event2/util.h>

void cmd_msg_cb(int fd, short events, void* arg);
void server_msg_cb(struct bufferevent* bev, void* arg);
void event_cb(struct bufferevent *bev, short event, void *arg);


int main(int argc, char** argv)
{
	if(argc<3)
	{
		printf("2  paramaters");
		return -1;
	}
	
	struct event_base* base=event_base_new();
	
	struct bufferevent* bev=bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	
	struct event* ev_cmd = event_new(base, STDIN_FILENO, EV_READ | EV_PERSIST, cmd_msg_cb, (void*)bev);
	
	event_add(ev_cmd, NULL);
	
	struct sockaddr_in server_addr;
	
	memset(&server_addr, 0, sizeof(server_addr));
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &server_addr.sin_addr);
	
	bufferevent_socket_connect(bev, (struct sockaddr*)&server_addr, sizeof(server_addr));
	
	bufferevent_setcb(bev, server_msg_cb, NULL, event_cb, (void*)ev_cmd);
	bufferevent_enable(bev, EV_READ | EV_PERSIST);
	
	event_base_dispatch(base);
	
	return 0;
}

void cmd_msg_cb(int fd, short events, void* arg)
{
	char msg[1024];
	
	int ret=read(fd, msg, sizeof(msg));
	if(ret<0)
	{
		perror("read failed");
		exit(1);
	}
	
	struct bufferevent *bev= (struct bufferevent*)arg;
	
	bufferevent_write(bev, msg, ret);
}

void server_msg_cb(struct bufferevent* bev, void* arg)
{
	char msg[1024];
	
	size_t len=bufferevent_read(bev, msg, sizeof(msg));
	msg[len]='\0';
	
	printf("recv %s from server\n", msg);
}

void event_cb(struct bufferevent* bev, short events, void* arg)
{
	if(events  &  BEV_EVENT_EOF)
		printf("connection close\n");
	else if(events & BEV_EVENT_ERROR)
		printf("some other error\n");
	else if(events & BEV_EVENT_CONNECTED)
	{
		printf("connected ok\n");
		return ;
	}		
	
	bufferevent_free(bev);
	
	struct event* ev=(struct event*)arg;
	event_free(ev);
}

