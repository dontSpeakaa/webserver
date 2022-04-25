#include "server.h"


const int waitTime = -1;

const int MAX_fd = 1024;

//预先为每个可能的客户连接分配一个http_conn对象
httpConn* users = new httpConn[ MAX_fd ];

int user_count = 0;


struct fds
{
	int fd;
	epoller* epoll;
};

server::server( int port )
: _port( port ), _listenfd( -1 ), _epoll(new epoller),
  _pool( new threadpool )
{ 

}

server::~server() { 
	delete _pool;
}

void server::initServer()
{	
	_listenfd = socket( PF_INET, SOCK_STREAM, 0 );
	assert( _listenfd > 0 );

	sockaddr_in address;
	bzero( &address, sizeof( address ) );
	address.sin_family = AF_INET;
	address.sin_port = htons( _port );
	address.sin_addr.s_addr = htonl( INADDR_ANY );

	int ret = bind( _listenfd, ( struct sockaddr* )&address, sizeof( address ) );
	assert( ret != -1 );

	ret = listen( _listenfd, 5 );
	assert( ret != -1 );

	//将listen加入epoller
	bool flag = _epoll->addfd( _listenfd, false );
	assert( flag );

}

void server::startServer()
{
	initServer();
	std::cout << "starting" << std::endl;
	
	while(1)
	{
		int num = _epoll->waitNum( waitTime );
		if( num < 0 )
		{
			std::cout << "error: " << errno << std::endl;
			break;
		}

		for( int i = 0; i < num; ++i )
		{
			int sockfd = _epoll->getfd( i );
			uint32_t event = _epoll->getEvent( i );

			if( sockfd == _listenfd )
			{
				handleConnection();
			}
			else if( event & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) )
			{
				exit(0);
			}
			else if( event & EPOLLIN )
			{
				_pool->append( users + sockfd );
				//_epoll->reset_oneshot( sockfd );
			}
			else
			{
				std::cout << "something wrong happened" << std::endl;
			}
		}
	}
}

void server::handleConnection()
{
	std::cout << "beginning listening" << std::endl;
	struct sockaddr_in client_address;
	socklen_t client_addrlength = sizeof( client_address );
	int connfd = accept( _listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
	
	_epoll->addfd( connfd, true );								//确保最多触发一个注册的事件
	setnonblocking( connfd );									//且只触发一次
	users[connfd].init( connfd, client_address, _epoll);
}																

// void server::handleEvent( int fd )
// {
// 	//分配线程
// 	// pthread_t thread;
// 	// fds fds_for_new_work;
// 	// fds_for_new_work.fd = fd;
// 	// fds_for_new_work.epoll = _epoll;
// 	// pthread_create( &thread, NULL, worker, ( void* )&fds_for_new_work );
// 	// std::cout << "trigger once" << std::endl;
// }

// void* server::worker( void* arg )
// {
// 	int fd = (( fds* )arg)->fd;
// 	epoller* epoller = (( fds* )arg)->epoll;
// 	std::cout << "start new thread to receive data on " << fd << std::endl;
// 	while(1)
// 	{
// 		char buf[ BUFFER_SIZE ];
// 		memset( buf, '\0', BUFFER_SIZE );
// 		int ret = recv( fd, buf, BUFFER_SIZE-1, 0 );
// 		if( ret < 0 )
// 		{
// 			//对于非阻塞IO，下面的条件成立 表示数据已经全部读取完成，此后	
// 			//epoll就能再次触发sockfd上的epollin事件，以驱动下一次读操作
// 			if(  ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) )
// 			{
// 				epoller->reset_oneshot( fd );
// 				std::cout << "read later" << std::endl;
// 				break;
// 			}
// 			close( fd );
// 			break;
// 		}
// 		else if( ret == 0 )
// 		{
// 			close( fd );
// 			std::cout << "foreiner closed the connection" << std::endl;
// 			break;
// 		}
// 		else
// 		{
// 			std::cout << "get" << ret << "bytes of content: " << buf << std::endl;
// 		}
// 	}
// 	std::cout << "end thread to receive data on " << fd << std::endl;
// 	return nullptr;
// }

void server::setnonblocking( int fd )
{
	int old_option = fcntl( fd, F_GETFL );
	fcntl( fd, F_SETFL, old_option | O_NONBLOCK );
}

