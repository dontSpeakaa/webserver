#ifndef SERVER_H
#define SERVER_H 

#include "epoller/epoller.h"
#include "threadpool/threadpool.h"
#include "httpConn.h"
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/un.h>
#include <pthread.h>


class server{
public:
	server( int port = 10000 );
	~server();

	void initServer();
	void startServer();

	void handleConnection();
	void handleEvent( int fd );

	//向客户端发送错误
	void send_error( int connfd, const char* info );


	void setnonblocking( int fd ); 	//设置为非阻塞


private:
	int _port;
	int _listenfd;	//将服务器主线程用作监听
	epoller* _epoll;
	threadpool* _pool;	//线程池

};

#endif

//int pthread_create( pthread_t* thread, const pthread_attr_t* attr, 
//					void* ( *start_routine) (void), void* arg );