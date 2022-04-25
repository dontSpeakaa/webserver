#include "server.h"


const int waitTime = -1;

const int MAX_fd = 1024;
const int TIMESLOT = 5;
static bool timer_out = false;

static int pipefd[2];

//预先为每个可能的客户连接分配一个http_conn对象
httpConn* users = new httpConn[ MAX_fd ];

int user_count = 0;


struct fds
{
	int fd;
	epoller* epoll;
};

void cb_func( int fd )	
{
	
	close( fd );
	printf("(close fd %d)\n", fd);
}	

server::server( int port )
: _port( port ), _listenfd( -1 ), _epoll(new epoller),
  _pool( new threadpool ), _timer_list( new timer_list )
{ 

}

server::~server() { 
	delete _epoll;
	delete _pool;
	delete _timer_list;
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
	add_fd( _listenfd, false );

	//设置管道
	ret = socketpair( PF_UNIX, SOCK_STREAM, 0, pipefd );
	assert( ret != -1 );
	_epoll->setnonblocking( pipefd[1] );
	add_fd( pipefd[0], false );
	//设置信号
	add_sig( SIGALRM );

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
			else if( sockfd == pipefd[0] & ( event & EPOLLIN ) )
			{
				std::cout << "Sig event" << std::endl;
				handlerSig( timer_out );
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
			if( timer_out )
			{
				_timer_list->tick();
				alarm( TIMESLOT );
				timer_out = false;
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
	//创建定时器，设置其回调函数和超时时间，然后绑定定时器与用户，最后将定时器添加到链表中
	util_timer* timer = new util_timer;
	time_t cur = time( NULL );
	timer->expire = cur + 3 * TIMESLOT;
	timer->fd = connfd;
	timer->cb_func = cb_func;
	_timer_map[connfd] = timer;
	_timer_list->add_timer( timer );
	user_count++;
}	

													



void server::send_error( int connfd, const char* info )
{
	std::cout << info << std::endl;
	send( connfd, info, strlen( info ), 0 );
	close( connfd );
}

void server::handlerSig( bool& timerout )
{
	char signals[1024];
	int ret = recv( pipefd[0], signals, sizeof( signals ), 0 );
	if( ret == -1 )
	{
		std::cout << "signal error" << std::endl;
	}
	else if( ret == 0 )
	{
		return ;
	}
	else 
	{
		for( int i = 0; i < ret; ++i )
		{
			if( signals[i] == SIGALRM )
			{
				timer_out = true;
				break;
			}
		}
	}
}

void sig_handler( int sig )
{
	int save_errno = errno;
	int msg = sig;
	send( pipefd[1], ( char* )&msg, 1, 0 );
	errno = save_errno;
}

void server::add_sig( int sig )
{
	struct sigaction sa;
	memset( &sa, '\0', sizeof( sa ) );
	sa.sa_handler = sig_handler;
	sa.sa_flags |= SA_RESTART;
	sigfillset( &sa.sa_mask );
	assert( sigaction( sig, &sa, NULL ) != 0 );
}

void server::timer_handler()
{
	_timer_list->tick();
	alarm( TIMESLOT );
}

void server::add_fd( int fd, bool isOneShot )
{
	assert( _epoll->addfd( fd, isOneShot ) == true );
}

void server::mod_fd( int fd, uint32_t events )
{
	assert( _epoll->modfd( fd, events ) == true );
}

void server::del_fd( int fd )
{
	assert( _epoll->delfd( fd ) == true );
}