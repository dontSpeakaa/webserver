#include "httpConn.h"

const int BUFFER_SIZE = 1024;

httpConn::httpConn()
: m_connfd( -1 ),  m_epoll( nullptr )
{

}

httpConn::~httpConn() { }

void httpConn::init( int connfd, const sockaddr_in& address, epoller* epoll )
{
	m_connfd = connfd;
	m_address = address;
	m_epoll = epoll;
}

// void httpConn::init()
// {

// }

void httpConn::process(){
	std::cout << "start new thread to receive data on " << m_connfd << std::endl;
  	while(1)
  	{
  		char buf[ BUFFER_SIZE ];
  		memset( buf, '\0', BUFFER_SIZE );
  		int ret = recv( m_connfd, buf, BUFFER_SIZE-1, 0 );
  		if( ret < 0 )
  		{
  			//对于非阻塞IO，下面的条件成立 表示数据已经全部读取完成，此后	
  			//epoll就能再次触发sockfd上的epollin事件，以驱动下一次读操作
  			if(  ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) )
  			{
  				m_epoll->reset_oneshot( m_connfd );
  				std::cout << "read later" << std::endl;
  				break;
  			}
  			close( m_connfd );
  			break;
  		}
  		else if( ret == 0 )
  		{
  			close( m_connfd );
  			std::cout << "foreiner closed the connection" << std::endl;
  			break;
  		}
  		else
  		{
  			std::cout << "get " << ret << " bytes of content: " << buf;
  		}
  	}
  	std::cout << "end thread to receive data on " << m_connfd << std::endl << std::endl;
}