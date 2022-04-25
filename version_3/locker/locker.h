#ifndef LOCKER_H
#define LOCKER_H 

#include <exception>
#include <pthread.h>
#include <semaphore.h>

//封装信号量的类
class sem
{
public:
	sem()
	{
		if( sem_init( &m_sem, 0, 0 ) != 0 )
		{
			throw std::exception();
		}
	}

	~sem()
	{
		sem_destroy( &m_sem );
	}

	bool wait()
	{
		return sem_wait( &m_sem ) == 0;
	}

	bool post()
	{
		return sem_post( &m_sem ) == 0;
	}

private:
	sem_t m_sem;
	
};

//封装互斥锁的类
class locker
{
public:
	locker()
	{
		if( pthread_mutex_init( &m_mutex, NULL ) != 0 )
		{
			throw std::exception();
		}
	}
	~locker()
	{
		pthread_mutex_destroy( &m_mutex );
	}
	
	bool lock()
	{
		return pthread_mutex_lock( &m_mutex ) == 0;
	}

	bool unlock()
	{
		return pthread_mutex_unlock( &m_mutex ) == 0;
	}

private:
	pthread_mutex_t m_mutex;
};
#endif