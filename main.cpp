#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
//#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "io.h"
#include "request.h"

#define SERV_PORT 9999

int main(int argc, char* argv[]) 
{
	u_short port = SERV_PORT;
	if (argc == 2) 
		port = atoi(argv[1]);
	
	int server_sock = -1;
	u_short port = 0;
	int client_sock = -1;
	//sockaddr_in 是 IPV4的套接字地址结构。定义在<netinet/in.h>,参读《TLPI》P1202
	struct sockaddr_in client_name;
	int client_name_len = sizeof(client_name);

	server_sock = startup(&port);
	printf("httpd running on port %d\n", port);


	Close(server_sock);

	return(0);

}
