#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.h"

class http_conn
{
public:
    // max length of filename
	static const int FILENAME_LEN = 200;
	static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    // http request method. We only support GET
	enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };
    // when parsing requests, status of main Inifite Status Machine
	enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    // results of dealing with http requests
	enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    // status of reading line
	enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
    http_conn(){}
    ~http_conn(){}

public:
	// initialize new accepted connection
    void init( int sockfd, const sockaddr_in& addr );
    void close_conn( bool real_close = true );
    // process requests
	void process();
    bool read();
    bool write();

private:
	// initialize connection
    void init();
	// parse http request
    HTTP_CODE process_read();
    // fill http response
	bool process_write( HTTP_CODE ret );

	// these functions are used by process_read() to parse http requests
    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    char* get_line() { return m_read_buf + m_start_line; }
    LINE_STATUS parse_line();

	// these functions are used by process_write() to parse http response
    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();

public:
	// all events are on the only one epoll, so it is static
    static int m_epollfd;
	// number of users
    static int m_user_count;

private:
    int m_sockfd;
   	// client's socket address
	sockaddr_in m_address;

    char m_read_buf[ READ_BUFFER_SIZE ];
    int m_read_idx;
    int m_checked_idx;
    int m_start_line;
    char m_write_buf[ WRITE_BUFFER_SIZE ];
    int m_write_idx;
	
	// status of main ISM
    CHECK_STATE m_check_state;
    // method of request
	METHOD m_method;
	
    //char m_real_file[ FILENAME_LEN ];
    //char* m_url;
    // http protocal's version. We only support HTTP1.1
	char* m_version;
	// name of host machine
    char* m_host;
	// length of request's message
    int m_content_length;
	// if http request asks the connection is connected
    bool m_linger;

	/*
    char* m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
	*/
};

#endif
