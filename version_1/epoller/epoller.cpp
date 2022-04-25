#include "epoller.h"

epoller::epoller( int maxEvents ) 
: _epollfd( epoll_create( 512 ) ), _events( maxEvents )
{ 
	assert( _epollfd >= 0 &&  _events.size() > 0 );
}

epoller::~epoller() 
{
	close( _epollfd );
}

bool epoller::addfd( int fd, bool isOneShot )
{
	if( fd < 0 ) return false;
	epoll_event ev{ 0 };
	ev.data.fd = fd;
	ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	if( isOneShot )
	{
		ev.events |= EPOLLONESHOT;
	}
	return 0 == epoll_ctl( _epollfd, EPOLL_CTL_ADD, fd, &ev );
}

bool epoller::modfd( int fd, uint32_t events )
{
	if( fd < 0 ) return false;
	epoll_event ev{ 0 };
	ev.data.fd = fd;
	ev.events = events;
	return 0 == epoll_ctl( _epollfd, EPOLL_CTL_MOD, fd, &ev );
}

bool epoller::delfd( int fd )
{
	if( fd < 0 ) return false;
	epoll_event ev{ 0 };
	ev.data.fd = fd;
	return 0 == epoll_ctl( _epollfd, EPOLL_CTL_DEL, fd, &ev );
}

int epoller::waitNum( int timeOut )
{
	return epoll_wait( _epollfd, &_events[0], _events.size(), timeOut );
}


int epoller::getfd( size_t i ) const
{
	assert( i < _events.size() && i >= 0 );
	return _events[i].data.fd;
}


uint32_t epoller::getEvent( size_t i ) const
{
	assert( i < _events.size() && i >= 0 );
	return _events[i].events;
}

void epoller::reset_oneshot( int fd )
{
	epoll_event ev{ 0 };
	ev.data.fd = fd;
	ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	epoll_ctl( _epollfd, EPOLL_CTL_MOD, fd, &ev );
}