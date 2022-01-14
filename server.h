#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "locker.h"
#include "threadpool.h"
#include "my_request.h"
#include "log/log.h"
#include "pool/sqlconnpool.h"
#include "pool/sqlconnRAII.h"
#include "buffer/buffer.h"
//#include ""
//#include "sockfd.h"

class Server
{
	public:
		Server( int port, int sqlPort, const char* sqlUser, 
				const char* sqlPwd, const char*dbName, 
				int connPoolNum, 
				int requestNum, int threadNum, bool openLog, 
				int logLevel, int logQueSize );
		
		~Server();
		void start();
	
	private:
		int m_port;
		int m_listenfd;
		int m_epollfd;
		bool m_isClose;

		threadpool< my_request >* m_threadpool;
		my_request* m_users;
		std::unordered_map< std::string, int > *m_map;

};

#endif
