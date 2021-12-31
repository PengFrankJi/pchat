#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>

#include "myTools.h"

void my_err(const char* str) {
	perror(str);
	exit(1);
}

int Socket(int domain, int type, int protocol) {
	int ret = socket(domain, type, protocol);
	if (ret == -1) {
		my_err("socket error");
	}
	return ret;
}

int Bind(int sockfd, const struct sockaddr* my_addr, socklen_t addrlen) {
	int ret = bind(sockfd, my_addr, addrlen);
	if (ret == -1) {
		my_err("bind error");
	}
	return ret;
}

int Listen(int sockfd, int backlog) {
	int ret = listen(sockfd, backlog);
	if (ret == -1) {
		my_err("listen error");
	}
	return ret;
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
	int cfd = accept(sockfd, addr, addrlen);
	if (cfd == -1) {
		my_err("accept error");
	}
	return cfd;
}
int Connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen) {
	int ret = connect(sockfd, serv_addr, addrlen);
	if (ret == -1) {
		my_err("connect error");
	}
	return ret;
}

int Read(int fd, void *buf, size_t nbyte) {
	size_t n;

again:
	n = read(fd, buf, nbyte);
	if (n == -1) {
		if (errno == EINTR) {
			goto again;
		} else {
			return -1;
		}
	}
	return n;
}

int Write(int fd, void *buf, size_t nbyte) {
	size_t n;

again:
	n = write(fd, buf, nbyte);
	if (n == -1) {
		if (errno == EINTR) {
			goto again;
		} else {
			return -1;
		}
	}
	return n;
}
int Close(int fd) {
	int n = close(fd);
	if (n == -1) {
		my_err("close error");
	}
	return n;
}

