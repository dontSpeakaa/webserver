#ifndef THREADPOOL_H
#define THREADPOOL_H 

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
//线程同步机制包装类
#include "../locker/locker.h"
#include "../httpConn.h"

//线程池类
class threadpool
{
public:
	//参数thread_number是线程池中的线程数量，max_requests是请求队列中最多允许的、等待处理的请求数量
	threadpool( int thread_number = 8, int max_requests = 10000 );
	~threadpool();

	//往请求队列中添加任务
	bool append( httpConn* request );

private:
	//工作线程运行的函数，它不断从工作队列中取出任务并执行
	static void* worker( void* arg );
	void run();

private:
	int m_thread_number;	//线程池中的数量
	int m_max_requests;
	pthread_t* m_threads;	//描述线程池的数组，其大小为m_thread_number
	std::list< httpConn* > m_workqueue;	//请求队列
	locker m_queuelocker;			//保护请求队列的互斥锁
	sem m_queuestat;				//是否有任务需要处理
	bool m_stop;					//是否结束线程
};

#endif