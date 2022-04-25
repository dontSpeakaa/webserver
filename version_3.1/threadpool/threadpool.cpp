#include "threadpool.h"

threadpool::threadpool( int thread_number, int max_requests )
: m_thread_number( thread_number ), m_max_requests( max_requests ),
  m_stop( false )//, m_threads( nullptr )
{ 
	std::cout << "Creating Threadpoll..." << std::endl << std::endl;
	if( ( thread_number <= 0 ) || ( max_requests <= 0 ) )
	{
		throw std::exception();
	}
	//m_threads = new pthread_t[ m_thread_number ];
	// if( !m_threads )
	// {
	// 	throw std::exception();
	// }

	//创建thread_number个线程，并将它们设置为脱离线程
	m_threads.resize( m_thread_number );
	for( int i = 0; i < thread_number; ++i )
	{
		//std::cout << "create the " << i << "th thread" << std::endl;
		if( pthread_create( &m_threads[i], nullptr, worker, this ) != 0 )
		{
			//delete [] m_threads;
			throw std::exception();
		}
		// if( pthread_detach( &m_threads[i] ) )
		// {
		// 	//delete [] m_threads;
		// 	throw std::exception();
		// }
	}
}


threadpool::~threadpool()
{
	//delete [] m_threads;
	m_stop = true;
}


bool threadpool::append( httpConn* request )
{
	//操作工作队列时一定要加锁，因为它被所有线程共享
	m_queuelocker.lock();
	if( m_workqueue.size() > m_max_requests )
	{
		m_queuelocker.unlock();
		return false;
	}
	m_workqueue.push_back( request );
	m_queuelocker.unlock();
	m_queuestat.post();
	return true;
}


void* threadpool::worker( void* arg )
{
	threadpool* pool = ( threadpool* )arg;
	pool->run();
	return pool;
}


void threadpool::run()
{
	while( !m_stop )
	{
		m_queuestat.wait();
		m_queuelocker.lock();
		if( m_workqueue.empty() )
		{
			m_queuelocker.unlock();
			continue;
		}
		httpConn* request = m_workqueue.front();
		m_workqueue.pop_front();
		m_queuelocker.unlock();
		if( !request )
		{
			continue;
		}
		request->process();
	}
}


