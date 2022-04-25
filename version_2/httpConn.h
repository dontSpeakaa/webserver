#ifndef HTTPCONN_H
#define HTTPCONN_H 

#include "epoller/epoller.h"
#include <sys/un.h>

#include <sys/socket.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <bits/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <stdarg.h>
#include <sys/uio.h>
#include <unistd.h>       
#include <sys/mman.h>
#include<sys/stat.h>

class httpConn
{
public:

	static const int READ_BUFFER_SIZE = 2048;   //读缓冲区大小
	static const int WRITE_BUFFER_SIZE = 1024; //写缓冲区
	static const int FILENAME_LEN = 200;

	//主状态机的两种可能状态
	enum CHECK_STATE
	{
		CHECK_STATE_REQUESTLINE = 0,
		CHECK_STATE_HEADER,
		CHECK_STATE_CONTENT
	};

	//从状态机三种可能状态，读取到一个完整的行、行出错、行数据不完整
	enum LINE_STATUS
	{
		LINE_OK = 0,
		LINE_BAD,
		LINE_OPEN
	};

	/*服务器处理http请求的结果： NO_REQUEST表示请求不完整，需要继续读取客户数据； 
	GET_REQUEST表示获得一个完整的客户数据；
	BAD_REQUEST表示客户端请求有语法错误；
	FORBIDDEN_REQUEST表示客户对资源没有足够的访问权限
	INTERNAL_ERROR表示服务器内部错误
	CLOSED_CONNECTION表示客户端已经关闭连接
	*/
	enum HTTP_CODE
	{
		NO_REQUEST = 0,
		GET_REQUEST,
		BAD_REQUEST,
		NO_RESOURCE,
		FORBIDDEN_REQUEST,
		FILE_REQUEST,
		INTERNAL_ERROR,
		CLOSED_CONNECTION 
	};

	enum METHOD
	{
		GET = 0,
		POST,
		HEAD,
		PUT,
		DELETE,
		TRACE,
		OPTIONS,
		CONNECT,
		PATCH
	};


public:
	httpConn();
	~httpConn();
	//初始化新接受的连接
	void init( int connfd, const sockaddr_in& address, epoller* epoll );
	

	//循环读取客户数据，直到无数据可读或者对方关闭连接
	bool read();
	bool write();

	//处理客户请求
	void process();
	void disconnect();
//private:
	// void init();
private:
	int m_connfd;
	sockaddr_in m_address;
	//将httpconn对应的注册表复制进来
	epoller* m_epoll;

private:
	int m_data_read;
	int m_read_index;						//当前已经读取了多少字节的客户数据
	int m_write_index;
	int m_checked_index;					//当前行已经分析了多少字节的客户数据
	int m_start_line;						//行在buffer的起始位置
	char m_read_buff[ READ_BUFFER_SIZE ];	//读缓冲区
	char m_write_buff[ WRITE_BUFFER_SIZE ];
	int m_content_length;					//http请求消息体的长度
	METHOD m_method;
	char* m_url;
	char* m_version;
	char* m_host;
	bool m_linger;							//是否长连接
	char  m_real_file[ FILENAME_LEN ];		//客户请求的目标文件名
	char* m_file_address;					//客户请求的目标文件被mmap到内存中的起始位置

	CHECK_STATE m_check_state;				//状态机状态

	//目标文件的状态。通过它我们可以判断文件是否存在、是否为目录、是否可读，并获取文件大小等信息
	struct stat m_file_stat;
	//我们采用writev来执行写操作
	struct iovec m_iv[2];
	int m_iv_count;									//struct iovec
													//{
													//		void *iov_base   内存起始地址
													//		size_t iov_len   这块内存的长度
													//}
private:
	void init();
	//从状态机，用于解析出一行的内容
	LINE_STATUS parse_line( );
	//分析请求行
	HTTP_CODE parse_requestline( char* temp );
	//分析头部字段
	HTTP_CODE parse_headers( char* temp );
	//分析http请求的入口函数

	HTTP_CODE do_request();
	HTTP_CODE parse_content( char* text );
	HTTP_CODE process_read();
	bool process_write( HTTP_CODE ret );
	
	char* get_line() { return m_read_buff + m_start_line; }

	//下面的函数被process_write调用以填充http应答
	void unmap();
	bool add_response( const char* format, ... );
	bool add_content( const char* content );
	bool add_status_line( int status, const char* title );
	bool add_headers( int content_length );
	bool add_file_type();
	bool add_content_length( int content_length );
	bool add_linger();
	bool add_blank_line();

	//获取文件属性
	const char* get_file_type( const char* name );




};

#endif