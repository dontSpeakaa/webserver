#ifndef LST_TIMER
#define LST_TIMER 

#include <iostream>
#include "../epoller/epoller.h"
#include "../logs/log.h"
#define BUFFER_SIZE 64


class util_timer
{
public:
	util_timer() : prev( nullptr ), next( nullptr ) { 	}
	
public:
	time_t expire;			//任务超时时间
	void ( *cb_func ) ( epoller*, int );		//任务回调函数
	//回调函数处理的客户数据，由定时器的执行者传递给回调函数
	int fd;
	epoller* epoll;
	util_timer* prev;
	util_timer* next; 
};

class timer_list
{
public:
	timer_list();
	~timer_list();

	//将目标定时器添加到链表中
	void add_timer( util_timer* timer );
	//当某个定时任务发生变化时，调整对应的定时器在链表中的位置
	void adjust_timer( util_timer* timer );
	//将目标定时器从链表中删除
	void del_timer( util_timer* timer );
	//void timer_handler();
	void tick();
private:
	//将定时器添加到节点lst_head之后的部分链表中
	void add_timer( util_timer* timer, util_timer* lst_head );
	
private:
	util_timer* head;
	util_timer* tail;
};

#endif