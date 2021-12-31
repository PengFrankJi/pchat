#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>

void my_err(const char* str);

int Socket(int domain, int type, int protocol);

int Bind(int sockfd, const struct sockaddr* my_addr, socklen_t addrlen);

int Listen(int sockfd, int backlog);

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int Connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);

int Read(int fd, void *buf, size_t nbyte);

int Write(int fd, void *buf, size_t nbyte);

int Close(int fd);

