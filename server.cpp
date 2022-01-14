#include "server.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000

extern int addfd( int epollfd, int fd, bool one_shot, bool in );
extern int removefd( int epollfd, int fd );

void addsig( int sig, void( handler )(int), bool restart = true )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    if( restart )
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}

void show_error( int connfd, const char* info )
{
    printf( "%s", info );
    send( connfd, info, strlen( info ), 0 );
    close( connfd );
}


Server::Server( int port, int sqlPort, const char* sqlUser, 
				const char* sqlPwd, const char*dbName, 
				int connPoolNum,
				int requestNum, int threadNum, bool openLog, 
				int logLevel, int logQueSize ):
	m_port(port), m_isClose(false), m_threadpool(new threadpool<my_request>(threadNum))
{
	m_users = new my_request[ requestNum ];
	m_map = new std::unordered_map< std::string, int >() ;
	SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

	if ( openLog )
	{
		Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
		if ( m_isClose ) {
			LOG_ERROR("========= Server init error =========");
		} else {
			LOG_INFO("========= Server init =========");
			LOG_INFO("Port:%d", m_port);
			//LOG_INFO("Listen Mode: %s, OpenConn:");
			LOG_INFO("LogSys level: %d", logLevel);
			LOG_INFO("sqlConn num: %d, ThreadPool num: %d", connPoolNum, threadNum);
		}
	}
}

void Server::start()
{
	m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if ( m_listenfd < 0 ){
		LOG_ERROR( "socket fail." );
	}
    assert( m_listenfd >= 0 );
	fcntl(m_listenfd, F_SETFL, O_NONBLOCK);

    int ret = 0;
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    //inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( m_port );

    ret = bind( m_listenfd, ( struct sockaddr* )&address, sizeof( address ) );
	if ( ret < 0 ) {
		LOG_ERROR( "bind fail." );
	}
    assert( ret >= 0 );

    ret = listen( m_listenfd, 5 );
    if ( ret < 0 ) {
		LOG_ERROR( "listen fail." );
	}
	assert( ret >= 0 );

    epoll_event events[ MAX_EVENT_NUMBER ];
    m_epollfd = epoll_create( 5 );
    if ( m_epollfd == -1 ) {
		LOG_ERROR( "epoll_create fail." );
	}
	assert( m_epollfd != -1 );
    addfd( m_epollfd, m_listenfd, false, true );
    my_request::m_epollfd = m_epollfd;

    while( true )
    {
		printf("epoll wait now ... \n");
        int number = epoll_wait( m_epollfd, events, MAX_EVENT_NUMBER, -1 );
        if ( ( number < 0 ) && ( errno != EINTR ) )
        {
            printf( "epoll failure\n" );
            break;
        }

        for ( int i = 0; i < number; i++ )
        {
            int sockfd = events[i].data.fd;
            if( sockfd == m_listenfd )
            {
				LOG_INFO( "new user comes." );
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof( client_address );
                int connfd = accept( m_listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
                if ( connfd < 0 )
                {
                    printf( "errno is: %d\n", errno );
                    LOG_ERROR( "listen fail." );
					continue;
                }
                if( my_request::m_user_count >= MAX_FD )
                {
                    show_error( connfd, "Internal server busy" );
                    LOG_WARN("Internal server busy");
					continue;
                }
                
                m_users[connfd].init( connfd, client_address, m_map, m_users );
            }
            else if( events[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) )
            {
                LOG_INFO("close connection to socket: %d", sockfd);
				m_users[sockfd].close_conn();
            }
            else if( events[i].events & EPOLLIN )
            {
                //printf("EPOLLIN.\n");
				LOG_INFO("EPOLLIN, read");
				if( m_users[sockfd].read() )
                {
                    m_threadpool->append( m_users + sockfd );
                }
                else
                {
                    m_users[sockfd].close_conn();
                }
            }
            else if( events[i].events & EPOLLOUT )
            {
                //printf("EPOLLOUT.\n");
                LOG_INFO("EPOLLOUT, write");
				if( !m_users[sockfd].write() )
                {
                    m_users[sockfd].close_conn();
                }
            }
            else
            {}
        }
    }
    //return 0;
}

Server::~Server() {
    close( m_epollfd );
    close( m_listenfd );
    delete [] m_users;
    delete m_threadpool;	
}

