#include "timer.h"

timer_list::timer_list()
: head( nullptr ), tail( nullptr )
{

}

timer_list::~timer_list()
{
	util_timer* temp = head;
	while( temp )
	{
		head = temp->next;
		delete temp;
		temp = head;
	}
}

void timer_list::add_timer( util_timer* timer )
{
	if( !timer )
	{
		return ;
	}
	if( !head )
	{
		head = tail = timer;
		return ;
	}
	if( timer->expire <= head->expire )
	{
		timer->next = head;
		head->prev = timer;
		head = timer;
		return ;
	}
	add_timer( timer, head );
}

void timer_list::adjust_timer( util_timer* timer )
{
	if( !timer )
	{
		return ;
	}
	util_timer* temp = timer->next;
	if( !temp || temp->expire >= timer->expire )
	{
		return ;
	}
	if( timer == head )
	{
		head = head->next;
		head->prev = nullptr;
		timer->next = nullptr;
		add_timer( timer, head );
	}
	else 
	{
		timer->prev->next = timer->next;
		timer->next->prev = timer->prev;
		add_timer( timer, timer->next );
	}
}

void timer_list::del_timer( util_timer* timer )
{
	if( !timer )
	{
		return ;
	}
	if( ( timer == head ) && ( timer == tail ) )
	{
		delete timer;
		head = nullptr;
		tail = nullptr;
		return ;
	}
	if( timer == head )
	{
		head = timer->next;
		head->prev = nullptr;
		delete timer;
		return ;
	}
	if( timer == tail )
	{
		tail = timer->prev;
		tail->next = nullptr;
		delete timer;
		return ;
	}
	timer->next->prev = timer->prev;
	timer->prev->next = timer->next;
	delete timer;
}


//sigalrm信号每次被触发就在其信号处理函数中执行一次tick函数，以处理链表上的到期任务
void timer_list::tick()
{
	if( !head )
	{
		return ;
	}
	std::cout << "timer tick" << std::endl;
	time_t cur = time( NULL );
	util_timer* temp = head;
	while( temp )
	{
		if( cur < temp->expire )
		{
			break;
		}
		//调用定时器回调函数，执行定时任务
		temp->cb_func( temp->epoll, temp->fd );
		head = temp->next;
		if( head ) 
		{
			head->prev = nullptr;
		}
		delete temp;
		temp = head;
	}
}

void timer_list::add_timer( util_timer* timer, util_timer* lst_head )
{
	util_timer* prev = lst_head;
	util_timer* tmp = prev->next;
	while( tmp )
	{
		if( timer->expire <= tmp->expire )
		{
			prev->next = timer;
			timer->next = tmp;
			tmp->prev = timer;
			timer->prev = prev;
			break;
		}
		prev = tmp;
		tmp = tmp->next;
	}
	//如果遍历完还没有找到
	if( !tmp )
	{
		prev->next = timer;
		timer->prev = prev;
		timer->next = nullptr;
		tail = timer;
	}
}