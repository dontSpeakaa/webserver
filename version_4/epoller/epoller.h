#ifndef EPOLLER_H
#define EPOLLER_H 

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>

class epoller
{
public:
	epoller( int maxEvents = 1024 );
	~epoller();
	bool addfd( int fd, bool isOneShot );
	bool addpipe( int pipefd );
	bool modfd( int fd, uint32_t events );
	bool delfd( int fd );
	int waitNum( int timeOut = -1 );
	int getfd( size_t i ) const;
	uint32_t getEvent( size_t i ) const;

	void setnonblocking( int fd );
	//void reset_oneshot( int fd );


private:
	int _epollfd;
	std::vector<epoll_event> _events;
};


#endif


// struct epoll_event
// {
// 	_uint32_t events;
// 	epoll_data_t data;	
// };

// typedef union epoll_data
// {
// 	void* ptr;
// 	int fd;
// 	uint32_t u32;
// 	uint64_t u64;
// } epoll_data_t;