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
	std::cout << "initial..." << std::endl;
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
	std::cout << "WebServer Start..." << std::endl << std::endl;
	
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
				std::cout << "One client joinning " << user_count << std::endl;
				std::cout << "Online Users : " << user_count << std::endl;
			}
			else if( event & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) )
			{
				users[sockfd].disconnect();
				user_count--;
				std::cout << "One client left " << user_count << std::endl;
				std::cout << "Online Users : " << user_count << std::endl;
			}
			else if( event & EPOLLIN )
			{
				if( users[sockfd].read() )			//循环读取客户数据，直到无数据可读或者对方关闭连接
				{
					_pool->append( users + sockfd );
				}
				else
				{

				}
			}
			else if( event & EPOLLOUT )
			{
				if( !users[sockfd].write() )
				{
					users[sockfd].disconnect();
					user_count--;
					std::cout << "One client left " << user_count << std::endl;
					std::cout << "Online Users : " << user_count << std::endl;
				}
			}
			else
			{
				
			}
		}
	}
	delete [] users;
	delete _pool;
	delete _epoll;
}


void server::handleConnection()
{
	struct sockaddr_in client_address;
	socklen_t client_addrlength = sizeof( client_address );
	int connfd = accept( _listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
	if( connfd < 0 )
	{
		std::cout << "errno is : " << errno << std::endl;
		return ;
	}
	if( user_count >= MAX_fd )
	{
		send_error( connfd, "Internal server busy\n");
		return ;
	}
	//初始化客户连接
	std::cout << "initial the client..." << std::endl;
	users[connfd].init( connfd, client_address, _epoll);
	user_count++;
}																



void server::send_error( int connfd, const char* info )
{
	std::cout << info << std::endl;
	send( connfd, info, strlen( info ), 0 );
	close( connfd );
}
