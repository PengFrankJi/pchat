#include "my_conn.h"

const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_400_form2 = "User you want to talk to cannot be found.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual internal error. Sorry for your inconvenience.\n";

int setnonblocking( int fd )
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}

void addfd( int epollfd, int fd, bool one_shot )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if( one_shot )
    {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
    setnonblocking( fd );
}

void removefd( int epollfd, int fd )
{
    epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, 0 );
    close( fd );
}

void modfd( int epollfd, int fd, int ev )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl( epollfd, EPOLL_CTL_MOD, fd, &event );
}

int my_conn::m_user_count = 0;
int my_conn::m_epollfd = -1;


void my_conn::close_conn( bool real_close )
{
    if( real_close && ( m_sockfd != -1 ) )
    {
        //modfd( m_epollfd, m_sockfd, EPOLLIN );
        removefd( m_epollfd, m_sockfd );
        m_sockfd = -1;
        m_user_count--;
    }
}

void my_conn::init( int sockfd, const sockaddr_in& addr )
{
    m_sockfd = sockfd;
    m_address = addr;
    int error = 0;
    socklen_t len = sizeof( error );
    getsockopt( m_sockfd, SOL_SOCKET, SO_ERROR, &error, &len );
    int reuse = 1;
    setsockopt( m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );
    addfd( m_epollfd, sockfd, true );
    m_user_count++;

    init();

	prepare_write_signlog();
}


void my_conn::prepare_write_signlog()
{

}

void my_conn::init()
{
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;

    m_method = GET;
    //m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    memset( m_read_buf, '\0', READ_BUFFER_SIZE );
    memset( m_write_buf, '\0', WRITE_BUFFER_SIZE );
    //memset( m_real_file, '\0', FILENAME_LEN );
}

my_conn::LINE_STATUS my_conn::parse_line()
{
    char temp;
    for ( ; m_checked_idx < m_read_idx; ++m_checked_idx )
    {
        temp = m_read_buf[ m_checked_idx ];
        if ( temp == '\r' )
        {
            if ( ( m_checked_idx + 1 ) == m_read_idx )
            {
                return LINE_OPEN;
            }
            else if ( m_read_buf[ m_checked_idx + 1 ] == '\n' )
            {
                m_read_buf[ m_checked_idx++ ] = '\0';
                m_read_buf[ m_checked_idx++ ] = '\0';
                return LINE_OK;
            }

            return LINE_BAD;
        }
        else if( temp == '\n' )
        {
            if( ( m_checked_idx > 1 ) && ( m_read_buf[ m_checked_idx - 1 ] == '\r' ) )
            {
                m_read_buf[ m_checked_idx-1 ] = '\0';
                m_read_buf[ m_checked_idx++ ] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }

    return LINE_OPEN;
}

bool my_conn::read()
{
    if( m_read_idx >= READ_BUFFER_SIZE )
    {
        return false;
    }

    int bytes_read = 0;
    while( true )
    {
        bytes_read = recv( m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0 );
        if ( bytes_read == -1 )
        {
            if( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                break;
            }
            return false;
        }
        else if ( bytes_read == 0 )
        {
            return false;
        }

        m_read_idx += bytes_read;
    }
    return true;
}


my_conn::HTTP_CODE my_conn::parse_request_line( char* text )
{
    m_url = strpbrk( text, " \t" );
    if ( ! m_url )
    {
        return BAD_REQUEST;
    }
    *m_url++ = '\0';

    char* method = text;
    if ( strcasecmp( method, "GET" ) == 0 )
    {
        m_method = GET;
    }
    else
    {
        return BAD_REQUEST;
    }

    m_url += strspn( m_url, " \t" );
    m_version = strpbrk( m_url, " \t" );
    if ( ! m_version )
    {
        return BAD_REQUEST;
    }
    *m_version++ = '\0';
    m_version += strspn( m_version, " \t" );
    if ( strcasecmp( m_version, "HTTP/1.1" ) != 0 )
    {
        return BAD_REQUEST;
    }
	
	/*
    if ( strncasecmp( m_url, "http://", 7 ) == 0 )
    {
        m_url += 7;
        m_url = strchr( m_url, '/' );
    }

    if ( ! m_url || m_url[ 0 ] != '/' )
    {
        return BAD_REQUEST;
    }
	*/

    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

my_conn::HTTP_CODE my_conn::parse_headers( char* text )
{
    if( text[ 0 ] == '\0' )
    {
        if ( m_method == HEAD )
        {
            return GET_REQUEST;
        }

        if ( m_content_length != 0 )
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }

        return GET_REQUEST;
    }
    else if ( strncasecmp( text, "Connection:", 11 ) == 0 )
    {
        text += 11;
        text += strspn( text, " \t" );
        if ( strcasecmp( text, "keep-alive" ) == 0 )
        {
            m_linger = true;
        }
    }
    else if ( strncasecmp( text, "Content-Length:", 15 ) == 0 )
    {
        text += 15;
        text += strspn( text, " \t" );
        m_content_length = atol( text );
    }
    else if ( strncasecmp( text, "Host:", 5 ) == 0 )
    {
        text += 5;
        text += strspn( text, " \t" );
        m_host = text;
    }
    else
    {
        printf( "oop! unknow header %s\n", text );
    }

    return NO_REQUEST;

}

// TODO
my_conn::HTTP_CODE my_conn::parse_content( char* text )
{
    if ( m_read_idx >= ( m_content_length + m_checked_idx ) )
    {
        text[ m_content_length ] = '\0';
		Json::Value val;
		Json::Reader read;
		if (read.parse(buf_all, val) == -1) {
			printf("json parse fail. Errno: %s\n", errno);
			return 0;
		}
		
		// get sender, reciver, message
		// strcpy(dest, val["reciver"].asCString());

		

		return GET_REQUEST;
    }

    return NO_REQUEST;
}

my_conn::HTTP_CODE my_conn::process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = 0;

    while ( ( ( m_check_state == CHECK_STATE_CONTENT ) && ( line_status == LINE_OK  ) )
                || ( ( line_status = parse_line() ) == LINE_OK ) )
    {
        text = get_line();
        m_start_line = m_checked_idx;
        printf( "got 1 http line: %s\n", text );

        switch ( m_check_state )
        {
            case CHECK_STATE_REQUESTLINE:
            {
                ret = parse_request_line( text );
                if ( ret == BAD_REQUEST )
                {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:
            {
                ret = parse_headers( text );
                if ( ret == BAD_REQUEST )
                {
                    return BAD_REQUEST;
                }
                else if ( ret == GET_REQUEST )
                {
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT:
            {
                ret = parse_content( text );
                if ( ret == GET_REQUEST )
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

// TODO
my_conn::HTTP_CODE my_conn::do_request()
{
	// modify reciver's fd to EPOLLOUT
	//std::string reciver_s( (const char*)reciver);
	if (name _fd_dic.count(reciver) == 0) {
		return NO_RECIVER;
	}
	int fd = name_fd_dic.at(reciver);
	
	// TODO: get form
	
	add_other_status_line(fd, 200, ok_200_title);
	add_other_headers(fd, strlen(form));
	add_other_content(form);

	return GET_REQUEST; 
}


// TODO
bool my_conn::write()
{
    int temp = 0;
    int bytes_have_send = 0;
    int bytes_to_send = m_write_idx;
    if ( bytes_to_send == 0 )
    {
        modfd( m_epollfd, m_sockfd, EPOLLIN );
        init();
        return true;
    }

    while( 1 )
    {
        //temp = writev( m_sockfd, m_iv, m_iv_count );
        temp = send( m_sockfd, m_write_buf + bytes_have_send, bytes_to_send, 0);
		if ( temp <= -1 )
        {
            if( errno == EAGAIN )
            {
                modfd( m_epollfd, m_sockfd, EPOLLOUT );
                return true;
            }
            return false;
        }

        bytes_to_send -= temp;
        bytes_have_send += temp;
        if ( bytes_to_send <= 0 )
        {
        	modfd( m_epollfd, m_sockfd, EPOLLIN );
            return true;
        }
    }
}

bool my_conn::add_response( const char* format, ... )
{
	if( m_write_idx >= WRITE_BUFFER_SIZE )
	{
		return false;
	}
	va_list arg_list;
	va_start( arg_list, format );
	int len = vsnprintf( m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list );
	if( len >= ( WRITE_BUFFER_SIZE - 1 - m_write_idx ) )
	{
		return false;
	}
	m_write_idx += len;
	va_end( arg_list );
	return true;
}

bool my_conn::add_status_line( int status, const char* title )
{
    return add_response( "%s %d %s\r\n", "HTTP/1.1", status, title );
}

bool my_conn::add_headers( int content_len )
{
    add_content_length( content_len );
    add_linger();
    add_blank_line();
}

bool my_conn::add_content_length( int content_len )
{
    return add_response( "Content-Length: %d\r\n", content_len );
}

bool my_conn::add_linger()
{
    return add_response( "Connection: %s\r\n", ( m_linger == true ) ? "keep-alive" : "close" );
}

bool my_conn::add_blank_line()
{
    return add_response( "%s", "\r\n" );
}

bool my_conn::add_content( const char* content )
{
    return add_response( "%s", content );
}


bool my_conn::add_other_response( int fd, const char* format, ... )
{	
	my_conn* conn = all_conns[fd];
	if( conn->m_write_idx >= WRITE_BUFFER_SIZE )
	{
		return false;
	}
	va_list arg_list;
	va_start( arg_list, format );
	int len = vsnprintf( conn->m_write_buf + conn->m_write_idx, WRITE_BUFFER_SIZE - 1 - conn->m_write_idx, format, arg_list );
	if( len >= ( WRITE_BUFFER_SIZE - 1 - conn->m_write_idx ) )
	{
		return false;
	}
	conn->m_write_idx += len;
	va_end( arg_list );
	return true;
}

bool my_conn::add_other_status_line( int fd, int status, const char* title )
{
    return add_other_response( fd, "%s %d %s\r\n", "HTTP/1.1", status, title );
}

bool my_conn::add_other_headers( int fd, int content_len )
{
    add_other_content_length( fd, content_len );
    add_other_linger( fd);
    add_other_blank_line(fd);
}

bool my_conn::add_other_content_length( int fd, int content_len )
{
    return add_other_response( fd, "Content-Length: %d\r\n", content_len );
}

bool my_conn::add_other_linger(int fd)
{
    return add_other_response( fd, "Connection: %s\r\n", ( m_linger == true ) ? "keep-alive" : "close" );
}

bool my_conn::add_other_blank_line(int fd)
{
    return add_other_response( fd, "%s", "\r\n" );
}

bool my_conn::add_other_content( int fd, const char* content )
{
    return add_other_response( int fd, "%s", content );
}

bool my_conn::process_write( HTTP_CODE ret )
{
    switch ( ret )
    {
        case INTERNAL_ERROR:
        {
            add_status_line( 500, error_500_title );
            add_headers( strlen( error_500_form ) );
            if ( ! add_content( error_500_form ) )
            {
                return false;
            }
            break;
        }
        case CONTENT_ERROR:
        {
            add_status_line( 400, error_400_title );
            add_headers( strlen( error_400_form ) );
            if ( ! add_content( error_400_form ) )
            {
                return false;
            }
            break;
        }
		case NO_RECIVER:
		{
            add_status_line( 400, error_400_title );
            add_headers( strlen( error_400_form2 ) );
            if ( ! add_content( error_400_form2 ) )
            {
                return false;
            }
            break;	
		}
        default:
        {
            return false;
        }
    }

    return true;
}

void my_conn::process()
{
    HTTP_CODE read_ret = process_read();
    if ( read_ret == NO_REQUEST )
    {
        modfd( m_epollfd, m_sockfd, EPOLLIN );
        return;
    }

    bool write_ret = process_write( read_ret );
    if ( ! write_ret )
    {
        close_conn();
    }

    modfd( m_epollfd, m_sockfd, EPOLLOUT );
}


}
