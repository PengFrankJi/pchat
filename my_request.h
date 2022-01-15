#ifndef MYREQUEST_H
#define MYREQUEST_H

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
//#include <jsoncpp/json/json.cpp>
#include <iostream>
#include <string>
#include <unordered_map>
//#include <mysql/mysql.h>
#include "locker.h"
#include "pool/sqlconnpool.h"
#include "pool/sqlconnRAII.h"
#include "log/log.h"


class my_request
{
	public:
		static const int READ_BUFFER_SIZE = 2048;
		static const int WRITE_BUFFER_SIZE = 2048;
		static const int USERNAME_SIZE = 10;
		static const int MESSAGE_SIZE = 50;
		
		// CONNECT: user is loging in or registering.
		// ALIVE: user can send and recive messages.
		enum STATUS {CONNECT, ALIVE};

		// NAME_PASSWORD: please input name and password
		// NAME_TOO_LONG
		// NAME_OCCUPIED
		// PASSWORD_INCORRECT
		// RECIVER_MESSAGE: input reciver and message
		// RECIVER_NOT_EXIST
		// MESSAGE_TOO_LONG
		enum RETURN_CODE {NAME_PASSWORD, NAME_TOO_LONG, NAME_OCCUPIED, PASSWORD_INCORRECT, RECIVER_MESSAGE, RECIVER_NOT_EXIST, MESSAGE_TOO_LONG, INTERNAL_ERROR};

	public:
		my_request() {}
		~my_request() {}

	public:
		void init(int sockfd, const sockaddr_in& addr, std::unordered_map<std::string, int> *map, my_request* all );
		void close_conn( bool real_close = true );
		void process();
		bool read();
		bool write();	

	private:
		void init();
		void process_write( RETURN_CODE code );
		void process_login( char* name, char* password );
		void process_message( char* reciver, char* message);


	public:
		static int m_epollfd;
		static int m_user_count;
		std::unordered_map<std::string, int> *m_name_map;

	private:
		int m_sockfd;
		sockaddr_in m_address;
		my_request* all_requests;
		
		STATUS m_status;
		std::string m_username;
		char m_read_buf[READ_BUFFER_SIZE];
		char m_write_buf[WRITE_BUFFER_SIZE];
		char m_reciver[USERNAME_SIZE];
		char m_message[MESSAGE_SIZE];

};
#endif
