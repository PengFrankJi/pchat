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
#include <jsoncpp/json/json.cpp>
#include <iostream>
#include <string>
#include "locker.h"

class my_conn
{
public:
	static const int READ_BUFFER_SIZE = 2048;
	static const int WRITE_BUFFER_SIZE = 2048;
	static const int USERNAME_SIZE = 10;
	static const int MESSAGE_SIZE = 50; 
	enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
	// NO_REQUEST: incomplete request, need to continue to read
	// GET_REQUEST: a complete request
	// BAD_REQUEST: there is syntax error
	// FORBIDDEN_REQUEST: user have no right
    //enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    enum HTTP_CODE { GET_REQUEST, CONTENT_ERROR, NO_RECIVER, INTERNEL_ERROR };
	enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
	my_conn() {}
	~my_conn() {}

public:
	void init(int sockfd, const sockaddr_in& addr );
	void close_conn( bool real_close = true )
    void process();
    bool read();
    bool write();

private:
    void init();
    HTTP_CODE process_read();
    bool process_write( HTTP_CODE ret );

    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    char* get_line() { return m_read_buf + m_start_line; }
    LINE_STATUS parse_line();

    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();

	bool add_other_response(int fd, const char* format, ... );
    bool add_other_content( int fd, const char* content );
    bool add_other_status_line( int fd, int status, const char* title );
    bool add_other_headers( int fd, int content_length );
    bool add_other_content_length( int fd, int content_length );
    bool add_other_linger(int fd);
    bool add_other_blank_line(int fd);
public:
	static int m_epollfd;
	static int m_user_count;
	static my_conn* all_conns;
	static unordered_map<string, int> name_fd_dic;
private:
    int m_sockfd;
    string username;
	sockaddr_in m_address;

    char m_read_buf[ READ_BUFFER_SIZE ];
    int m_read_idx;
    int m_checked_idx;
    int m_start_line;
    char m_write_buf[ WRITE_BUFFER_SIZE ];
    int m_write_idx;

    CHECK_STATE m_check_state;
    METHOD m_method;

    //char m_real_file[ FILENAME_LEN ];
	char* m_url;
    char* m_version;
    char* m_host;
    int m_content_length;
    bool m_linger;
	char reciver[USERNAME_SIZE];
	char message[MESSAGE_SIZE];

    //char* m_file_address;
    //struct stat m_file_stat;
    
	struct iovec m_iv[2];
    int m_iv_count;
}
#endif
