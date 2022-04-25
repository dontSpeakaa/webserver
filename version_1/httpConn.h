#ifndef HTTPCONN_H
#define HTTPCONN_H 

#include "epoller/epoller.h"
#include <sys/un.h>

#include <sys/socket.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <bits/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <pthread.h>
#include <iostream>
#include <unistd.h>

class httpConn
{
public:
	httpConn();
	~httpConn();
	//初始化新接受的连接
	void init( int connfd, const sockaddr_in& address, epoller* epoll );
	bool read();

	//处理客户请求
	void process();
//private:
	// void init();
private:
	int m_connfd;
	sockaddr_in m_address;
	//将httpconn对应的注册表复制进来
	epoller* m_epoll;

};

#endif