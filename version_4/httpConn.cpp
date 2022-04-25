#include "httpConn.h"



//定义HTTP响应的一些状态
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = 
            "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = 
            "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = 
            "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = 
            "There was an unusual problem serving the requested file.\n";

//网站根目录
const char* doc_root = "./";

httpConn::httpConn()
: m_connfd( -1 ),  m_epoll( nullptr )
{

}

httpConn::~httpConn() { }

void httpConn::init( int connfd, const sockaddr_in& address, epoller* epoll )
{
  //std::cout << std::endl << "initial httpconn" << std::endl;
	m_connfd = connfd;
	m_address = address;
	m_epoll = epoll;
  m_epoll->addfd( connfd, true );                 //确保最多触发一个注册的事件
  m_epoll->setnonblocking( connfd );              //且只触发一次

  init();
}

void httpConn::init()
{
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = true;

    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_index = 0;
    m_read_index = 0;
    m_write_index = 0;

    memset( m_read_buff, '\0', READ_BUFFER_SIZE );
    memset( m_write_buff, '\0', WRITE_BUFFER_SIZE );
    memset( m_real_file, '\0', FILENAME_LEN );
}

bool httpConn::read()   //循环读取客户数据，直到无数据可读或者对方关闭连接
{
    if( m_read_index > READ_BUFFER_SIZE )
    {
        return false;
    }
    int bytes_read = 0;
    while( true )
    {
        bytes_read = recv( m_connfd, m_read_buff + m_read_index, READ_BUFFER_SIZE - m_read_index, 0 );
        if( bytes_read == -1 )
        {
            if( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                break;
            }
            return false;
        }
        else if( bytes_read == 0 )
        {
            return false;
        }
        m_read_index += bytes_read;
    }
    return true;
}

//写http响应
bool httpConn::write()
{
    int temp = 0;
    int bytes_have_send = 0;
    int bytes_to_send = m_write_index;
    if( bytes_to_send == 0 )
    {
        m_epoll->modfd( m_connfd, EPOLLIN );
        init();
        return true;
    }

    while( 1 )
    {
        temp = writev( m_connfd, m_iv, m_iv_count );
        if( temp < -1 )
        {
            //如果tcp写缓冲没有空间，则等待下一轮EPOLLOUT事件，虽然在此期间
            //服务器无法立即接受到同一客户的下一个请求，但是可以保证连接的完整性
            if( errno == EAGAIN )
            {
                m_epoll->modfd( m_connfd, EPOLLOUT );
                return true;
            }
            unmap();
            return false;
        }
        bytes_to_send -= temp;
        bytes_have_send += temp;
        if( bytes_to_send <= bytes_have_send )
        {
            //发送http响应成功，根据http请求中的connection字段决定是否立即关闭连接
            unmap();
            if( m_linger )
            {
                init();
                m_epoll->modfd( m_connfd, EPOLLIN );
                return true;
            }
            else
            {
                m_epoll->modfd( m_connfd, EPOLLIN );
                return false;
            }
        }
    }
}


void httpConn::process(){
    HTTP_CODE read_ret = process_read();
    if( read_ret == NO_REQUEST )
    {
        m_epoll->modfd( m_connfd, EPOLLIN );
        //m_epoll->reset_oneshot( m_connfd );
        return ;
    }
    bool write_ret = process_write( read_ret );
    if( !write_ret )
    {
        disconnect();
    }
    m_epoll->modfd( m_connfd, EPOLLOUT );
    //m_epoll->reset_oneshot( m_connfd );
}

void httpConn::disconnect()
{
    m_epoll->delfd( m_connfd );
    close( m_connfd );
    m_connfd = -1;
}


/**************************************************************************************/
//用于分析请求
httpConn::LINE_STATUS httpConn::parse_line( )
{
  char temp;

  for( ; m_checked_index < m_read_index; ++m_checked_index )
  {
    temp = m_read_buff[ m_checked_index ];
    if(temp == '\r')
    {
      if( ( m_checked_index + 1 ) == m_read_index )
      {
        return LINE_OPEN;
      }
      else if( m_read_buff[ m_checked_index + 1 ] == '\n' )
      {
        m_read_buff[ m_checked_index++ ] = '\0';
        m_read_buff[ m_checked_index++ ] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    }
    else if( temp == '\n' )
    {
      if( (m_checked_index > 1) && m_read_buff[ m_checked_index - 1 ] == '\r' )
      {
        m_read_buff[ m_checked_index - 1 ] = '\0';
        m_read_buff[ m_checked_index++ ] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    }
  }
  return LINE_OPEN;
}

httpConn::HTTP_CODE httpConn::parse_requestline( char* temp )
{                              
  m_url = strpbrk( temp, " \t");  //返回搜索字符的指针  

  if( !m_url )
  {                             
    return BAD_REQUEST;
  }

  *m_url++ = '\0';

  char* method = temp;
  if( strcasecmp( method, "GET" ) == 0 ) //仅支持GET方法
  {
      m_method = GET;
      //std::cout << "The request method is GET" << std::endl;
  }
  else
  {
      return BAD_REQUEST;
  }

  //将url定位到 url 的位置，strspn返回url中第一次不匹配 " \t"字符的下标
  m_url += strspn( m_url, " \t" );
  m_version = strpbrk( m_url, " \t" );
  if( !m_version )
  {
      printf("no version\n");
      return BAD_REQUEST;
  }

  *m_version++ = '\0';
  m_version += strspn( m_version, " \t");

  //仅支持 HTTP/1.1
  if( strcasecmp( m_version, "HTTP/1.1" ) != 0 )
  {
      printf("bad version\n");
      return BAD_REQUEST;
  }

  //检查url是否合法
  if( strncasecmp( m_url, "http://", 7 ) == 0 )
  {
    m_url += 7;
    //在一个串中查找给定字符的第一个匹配之处，返回第一个匹配之处
    m_url = strchr( m_url, '/' );
  }

  if( !m_url || m_url[0] != '/' )
  {
    return BAD_REQUEST;
  }
  //std::cout << "The request URL is : " << m_url << std::endl;

  //http请求处理完毕，状态转移到头部字段分析
  m_check_state = CHECK_STATE_HEADER;
  return NO_REQUEST;
}

httpConn::HTTP_CODE httpConn::parse_headers( char* temp) 
{
  //如果遇到一个空行，说明我们得到了一个正确的http请求
  if( temp[0] == '\0' )
  {
      //如果HTTP请求有消息体，还需要读取m_content_length字节的消息，状态机转移到CHECK_STATE_CONTENT
    if( m_content_length != 0 )
    {
        m_check_state = CHECK_STATE_CONTENT;
        return NO_REQUEST;
    }
    return GET_REQUEST;
  }
  else if( strncasecmp( temp, "Connection:" , 11) == 0 )
  {    //处理HOST头部字段
    temp += 11;
    temp += strspn( temp, " \t");
    if( strcasecmp( temp, "keep-alive" ) == 0 )
    {
        m_linger = true;
    }
  }
  else if( strncasecmp( temp, "Content-Length:", 15 ) == 0 )
  {
      temp += 15;
      temp += strspn( temp, " \t" );
      m_content_length = atol( temp );
  }
  else if( strncasecmp( temp, "Host:", 5 ) == 0 )
  {
      temp += 5;
      temp += strspn( temp, " \t" );
      m_host = temp;
  }
  else
  {   //其它头部字段不处理
      //printf("opp! unknow header %s\n", temp );
  }
  return NO_REQUEST;
}


httpConn::HTTP_CODE httpConn::parse_content( char* text )
{
    if( m_read_index >= ( m_content_length + m_checked_index ) )
    {
        text[ m_content_length ] = '\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

httpConn::HTTP_CODE httpConn::process_read()
{
  LINE_STATUS line_status = LINE_OK;   //记录当前行的读取状态
  HTTP_CODE ret = NO_REQUEST;     //记录HTTP请求的处理结果
  char* text = 0;
  //主机状态机，用于从buffer中取出所有完整的行
  while( ( ( m_check_state == CHECK_STATE_CONTENT ) && ( line_status == LINE_OK ) ) 
          || ( ( (line_status = parse_line()) == LINE_OK ) ) )
  {
    text = get_line();
    m_start_line = m_checked_index;     //记录下一行的起始位置
    //printf("got 1 http line: %s\n", text );

    //checkstate记录主状态机当前的状态
    switch( m_check_state )
    {
      case CHECK_STATE_REQUESTLINE:
      {
        ret = parse_requestline( text );
        if( ret == BAD_REQUEST )
        {
          return BAD_REQUEST;
        }
        break;
      }
      case CHECK_STATE_HEADER:
      {
        ret = parse_headers( text );
        if( ret == BAD_REQUEST )
        {
          return BAD_REQUEST;
        }
        else if( ret = GET_REQUEST )
        {
          return do_request();
        }
        break;
      }
      case CHECK_STATE_CONTENT:
      {
        ret = parse_content( text );
        if( ret == GET_REQUEST )
        {
          return do_request();
        }
        line_status = LINE_OPEN;
        break;
      }
      default:
      {
        return INTERNAL_ERROR;
      }
    }
  }
  return NO_REQUEST;

}

//当得到一个完整、正确的http请求时，我们就分析目标文件的属性，对所有用户可读，且不是目录，则使用mmap
//将其映射到内存地址m_file_address处，并告诉调用者获取文件成功
httpConn::HTTP_CODE httpConn::do_request()
{
    strcpy( m_real_file, doc_root );
    int len = strlen( doc_root );
    strncpy( m_real_file + len, m_url, FILENAME_LEN - len - 1 );
    if( stat( m_real_file, &m_file_stat ) < 0 )
    {
        return NO_RESOURCE;
    }

    if( !( m_file_stat.st_mode & S_IROTH ) )
    {
        return FORBIDDEN_REQUEST;
    }

    if( S_ISDIR( m_file_stat.st_mode ) )
    {
        return BAD_REQUEST;
    }
    int fd = open( m_real_file, O_RDONLY );
    m_file_address = ( char* )mmap( 0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
    close( fd );
    return FILE_REQUEST;
}

//根据服务器处理http请求的结果，决定返回给客户端的内容
bool httpConn::process_write( HTTP_CODE ret )
{
    //printf("begin to write: ret = %d\n", ret);
    switch( ret )
    {
        case INTERNAL_ERROR:
        {
            add_status_line( 500, error_500_title );
            add_headers( strlen( error_500_form ) );
            if( !add_content( error_500_form ) )
            {
                return false;
            }
            break;
        }
        case BAD_REQUEST:
        {
            add_status_line( 400, error_400_title );
            add_headers( strlen( error_400_form ) );
            if( !add_content( error_400_form ) )
            {
                return false;
            }
            break;
        }
        case NO_RESOURCE:
        {
            add_status_line( 404, error_404_title );
            add_headers( strlen( error_404_form ) );
            if( !add_content( error_404_form ) )
            {
                return false;
            }
            break;
        }
        case FORBIDDEN_REQUEST:
        {
            add_status_line( 403, error_403_title );
            add_headers( strlen( error_403_form ) );
            if( !add_content( error_403_form ) )
            {
                return false;
            }
            break;
        }
        case FILE_REQUEST:
        {
            add_status_line( 200, ok_200_title );
            if( m_file_stat.st_size != 0 )
            {
                add_headers( m_file_stat.st_size );
                m_iv[0].iov_base = m_write_buff;
                m_iv[0].iov_len = m_write_index;
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                return true;
            }
            else 
            {
                const char* ok_string = "<html><body></body></html>";
                add_headers( strlen( ok_string ) );
                if( !add_content( ok_string ) )
                {
                    return false;
                }
            }
            break;
        }
        default:
        {
            return false;
        }
    }
    m_iv[0].iov_base = m_write_buff;
    m_iv[0].iov_len = m_write_index;
    m_iv_count = 1;
    return true;
}

//对内存映射区执行munmap操作
void httpConn::unmap()
{
    if( m_file_address )
    {
        munmap( m_file_address, m_file_stat.st_size );
        m_file_address = 0;
    }
}


//往写缓冲中写入待发送的数据
bool httpConn::add_response( const char* format, ... )
{
    if( m_write_index >= WRITE_BUFFER_SIZE )
    {
        return false;
    }
    va_list arg_list;
    va_start( arg_list, format );
    int len = vsnprintf( m_write_buff + m_write_index, WRITE_BUFFER_SIZE - 1 - m_write_index,
                         format, arg_list );
    if( len >= ( WRITE_BUFFER_SIZE - 1 - m_write_index ) )
    {
        return false;
    }
    m_write_index += len;
    va_end( arg_list );
    return true;
}

bool httpConn::add_status_line( int status, const char* title )
{
    return add_response( "%s %d %s\r\n", "HTTP/1.1", status, title );
}

bool httpConn::add_headers( int content_len )
{
    add_file_type();
    add_content_length( content_len );
    add_linger();
    add_blank_line();
    return true;
}

bool httpConn::add_file_type()
{
    return add_response( "Content-Type:%s\r\n", get_file_type( m_real_file ) );
}

bool httpConn::add_content_length( int content_len )
{
    return add_response( "Content-Length:%d\r\n", content_len );
}

bool httpConn::add_linger()
{
    return add_response( "Connection:%s\r\n", ( m_linger == true) ? "keep-alive" : "close" );
}

bool httpConn::add_blank_line()
{
    return add_response( "%s", "\r\n" );
}

bool httpConn::add_content( const char* content )
{
    return add_response( "%s", content );
}



const char* httpConn::get_file_type( const char* name )
{
    const char* dot;

    //从右向左查找'.'字符串，如果不存在，则返回null
    dot = strrchr( name, '.');
    if( dot == nullptr )
        return "text/plain; charset=utf-8";
    if( strcmp( dot, ".html") == 0 || strcmp( dot, ".htm" ) == 0 )
        return "text/html; charset=utf-8";
    if( strcmp( dot, ".jpg" ) == 0 || strcmp( dot, ".j[eg" ) == 0 )
        return "image/jpeg";
    if( strcmp( dot, ".gif" ) == 0 )
        return "image/gif";
    if( strcmp( dot, ".png" ) == 0 )
        return "image/png";
    if( strcmp( dot, ".css" ) == 0 )
        return "text/css";
    if( strcmp( dot, ".au" ) == 0 )
        return "audio/basic";
    if( strcmp( dot, ".wav" ) == 0 )
        return "audio/wav";
    if( strcmp( dot, ".avi" ) == 0 )
        return "video/x-msvideo";
    if( strcmp( dot, ".mov" ) == 0 )
        return "video/quicktime";
    if( strcmp( dot, ".mpeg" ) == 0 || strcmp( dot, ".mpe" ) == 0 )
        return "video/mpeg";
    if( strcmp( dot, ".vrml" ) == 0 || strcmp( dot, ".wrl" ) == 0 )
        return "model/vrml";
    if( strcmp( dot, ".midi" ) == 0 || strcmp( dot, ".mid" ) == 0 )
        return "audio/midi";
    if( strcmp( dot, ".mp3" ) == 0 )
        return "audio/mpeg";
    if( strcmp( dot, ".ogg" ) == 0 )
        return "application/ogg";
    if( strcmp( dot, ".pac" ) == 0 )
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}