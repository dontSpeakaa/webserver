#ifndef SERVER_H
#define SERVER_H 

#include "epoller/epoller.h"
#include "threadpool/threadpool.h"
#include "httpConn.h"
#include "timer/timer.h"
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
#include <unordered_map>
#include <signal.h>
#include <unistd.h>



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
	//信号相关
	void handlerSig( bool& timerout );
	//void sig_handler( int );
	void add_sig( int sig );
	void timer_handler();
	//void cb_func( int fd );



	//处理注册事件
	void add_fd( int fd, bool isOneShot );
	void mod_fd( int fd, uint32_t events );
	void del_fd( int fd );


private:
	int _port;
	int _listenfd;	//将服务器主线程用作监听
	epoller* _epoll;
	threadpool* _pool;	//线程池
	timer_list* _timer_list;	//定时器链表
	std::unordered_map<int, util_timer*> _timer_map;	//对定时器的映射

};

#endif

//int pthread_create( pthread_t* thread, const pthread_attr_t* attr, 
//					void* ( *start_routine) (void), void* arg );