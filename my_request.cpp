#include "my_request.h"
//#include "sockfd.h"

const char* login_s = "Please input your username and password in this form: [username]+[password]\nExample: Frank+123456\n";
const char* name_too_long = "Sorry, your username is too long.\n";
const char* name_occupied = "Sorry, your username is not available.\n";
const char* password_incorrect = "Sorry, your username or/and password is/are not correct.\n";
const char* request_message = "Please input reciver's username and message in this form: [reciver]+[message]\nExample: Frank+How are you?";
const char* reciver_not_exist = "Sorry, the reciver doesn't exist. Try some other people.\n";
const char* message_too_long = "Sorry, yout message is too long.\n";

int my_request::m_user_count = 0;
int my_request::m_epollfd = -1;

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
    event.events = EPOLLOUT | EPOLLET | EPOLLRDHUP;
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


void my_request::close_conn( bool real_close )
{
    if( real_close && ( m_sockfd != -1 ) )
    {
        //modfd( m_epollfd, m_sockfd, EPOLLIN );
        removefd( m_epollfd, m_sockfd );
        m_sockfd = -1;
        m_user_count--;
    }
}

void my_request::init( int sockfd, const sockaddr_in& addr, std::unordered_map<std::string, int> *map, my_request* all)
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
	m_name_map = map;
	all_requests = all;
    init();
	
	process_write( NAME_PASSWORD );

}

void my_request::process_login(char *name, char *password)
{
	std::string username(name);
	
	// TODO: check if name is occupied

	// TODO: check if password is correct

	m_username = username;
	m_status = ALIVE;
	std::unordered_map<std::string, int> map = *m_name_map;
	map[username] = m_sockfd;
	process_write( RECIVER_MESSAGE );
}

void my_request::process_message(char *reciver, char *message)
{
	std::string reci( reciver );
	// TODO: check if reciver exists. This is check in map. Need to check in database as well
	
	std::unordered_map<std::string, int> map = *m_name_map;
	std::unordered_map<std::string, int>::const_iterator got = map.find( reci );
	if ( got == map.end() ) // cannot find
	{
		process_write( RECIVER_NOT_EXIST );
	}

	// TODO: check if message if too long

	// lookup other my_request and change its EVENT to EPOLLOUT
	my_request* other = &(all_requests[ got->second ]);
	strcpy( other->m_write_buf, message );
	modfd(m_epollfd, got->second, EPOLLOUT);
}


void my_request::process()
{
	char* p;
	p = strpbrk( m_read_buf, "+" );
	p = '\0';
	p++;

	if ( m_status == CONNECT )
	{
		process_login(m_read_buf, p);
	}
	else
	{
		process_message(m_read_buf, p);
	}
	modfd( m_epollfd, m_sockfd, EPOLLOUT );
}

void my_request::process_write( RETURN_CODE code )
{
	memset( m_write_buf, '\0', WRITE_BUFFER_SIZE );
	switch ( code ) 
	{
		case NAME_PASSWORD:
		{
			strcpy( m_write_buf, login_s );
			break;
		}
		case NAME_TOO_LONG:
		{
			strcpy( m_write_buf, name_too_long );
			break;
		}
		case NAME_OCCUPIED:
		{
			strcpy( m_write_buf, name_occupied );
			break;
		}
		case PASSWORD_INCORRECT:
		{
			strcpy( m_write_buf, password_incorrect );
			break;
		}
		case RECIVER_MESSAGE:
		{
			strcpy( m_write_buf, request_message );
			break;
		}
		case RECIVER_NOT_EXIST:
		{
			strcpy( m_write_buf, reciver_not_exist );
			break;
		}
		case MESSAGE_TOO_LONG:
		{
			strcpy( m_write_buf, message_too_long );
			break;
		}
		default:
		{
			return;
		}
	}
}

void my_request::init()
{
	m_status = CONNECT;;

    memset( m_read_buf, '\0', READ_BUFFER_SIZE );
    memset( m_write_buf, '\0', WRITE_BUFFER_SIZE );
    memset( m_reciver, '\0', USERNAME_SIZE );
    memset( m_message, '\0', MESSAGE_SIZE );
}


bool my_request::read()
{
    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
	int bytes_read = 0;
    int tmp = 0;
	while( true )
    {
        tmp = recv( m_sockfd, m_read_buf + bytes_read, READ_BUFFER_SIZE - bytes_read, 0 );
        if ( tmp == -1 )
        {
            if( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                break;
            }
            return false;
        }
        else if ( tmp == 0 )
        {
            break;
        }

        bytes_read += tmp;
    }
	if (bytes_read == 0) 
		return false;
    return true;
}


bool my_request::write()
{
    int temp = 0;
    int bytes_have_send = 0;
    int bytes_to_send = strlen(m_read_buf);
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


