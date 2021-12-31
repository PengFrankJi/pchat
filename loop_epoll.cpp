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
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <jsoncpp/json/json.h>

#include "loop_epoll.h"
#include "io.h"

#define USER_LIMIT 10
#define BUF_SIZE 64
#define NAME_SIZE 20

typedef struct taginfo
{
	//struct sockaddr_in address;
	int fd;
	char name[NAME_SIZE];
	char* write_buf;
	char buf[BUF_SIZE];
} info;

int setnonblocking(int epollfd, int fd, bool, enable_et) {
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

void addfd(int epollfd, int fd, bool enable_oneshot) {
	epoll_event event;
	info *info_ptr;
	info_ptr->fd = fd
	event.data.ptr = info_ptr;
	event.events = EPOLLIN | EPOLLET;
	if (enable_oneshot)
	{
		event.events |= EPOLLONESHOT;
	}
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}

void reset_oneshot(int epollfd, int fd)
{
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

/*
void* worker(void* arg)
{
	int sockfd = ((fds*)arg)->sockfd;
	int epollfd = ((fds*)arg)->epollfd;
	printf("start new thread to receive data on fd: %d\n", sockfd);
	char buf[BUF_SIZE];
	memset(buf, '\0', BUF_SIZE);
	while
}
*/

void get_dest_and_message(int fd, char* buf, char* dest) {
	char buf_all[BUF_SIZE];
	int ret = Read(fd, buf_all, sizeof(buf_all));
	/*
	// already done in Read
	if (ret == -1) {
		my_err("fail to read from fd");
	}
	*/

	Json::Value val;
	//Json::Value root;
	Json::Reader read;
	if (-1 == read.parse(buf_all, val)) {
		printf("json parse fail. Errno: %s\n", errno);
		return 0;
	}
	
	strcpy(dest, val["dest"].asCString());
	strcpy(buf, val["val"].asCString());
}

void point_message(char* dest, epoll_event* events, int ret) {
	int i;
	for (i = 0; i < ret; i++) {
		if (events[i]
}

void et(struct epoll_event* events, int ret, int epollfd, int *user_counter, int listenfd) 
{
	int i;
	for (i = 0; i < ret; i++)
	{
		// If it is the listen file descriptor, accept the new user.
		if (events[i].data.ptr->fd == listenfd)
		{
			struct sockaddr_in clie_addr;
			socklen_t clie_addr_len = sizeof(clie_addr);
			int cfd = Accept(lfd, (struct sockaddr*)&clie_addr, &clie_addr_len);	
			
			// If number of users is smaller than limit, accept it
			if (user_counter < USER_LIMIT)
			{	
				addfd(epollfd, cfd, true);
				printf("Comes a new user.\n");
				user_counter++;

				//((info *)(events[i].data
			}
			else
			{
				char* message = "Too many users now.\n";
				printf("%s", message);
				Write(cfd, message, sizeof(message));
				Close(cfd);
				//continue;
			}
		}
		// It is not the listen file descriptor and it is EPOLLIN
		else if (events[i].events & EPOLLIN)
		{
			info *p_info = ((info *)(events[i].data.ptr));
			memset( p_info->buf, '\0', sizeof(p_info->buf));
			
			char dest[BUF_SIZE];
			memset(dest, '\0', sizeof(dest));
			get_dest_and_message(p_info->fd, p_info->buf, dest);
			
			// make the destination users' write_buf point to the message
			point_message(dest, epollfd);

		}

		// It is not the listen file descriptor and it is EPOLLOUT
		else if (events[i].events & EPOLLOUT)
		{
			if (! events[i].data.ptr->write_buf)
			{
				continue;
			}

			int r = Write(events[i].data.ptr->fd, events[i].data.ptr->write_buf, strlen(events[i].data.ptr->write));
			events[i].data.ptr->write_buf = NULL;
			
			events[i].events = EPOLLIN | EPOLLET | EPOLLONESHOT;
			epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &events[i]);
		}
	}
}

int epoll(int listenfd, ) 
{
	struct epoll_event events[USER_LIMIT + 1];  // users and listenfd
	int epollfd = epoll_create(USER_LIMIT + 1);
	addfd(epollfd, listenfd, true);
	
	int ret;
	int user_counter = 0;
	
	// initialize the data structure: fd_to_user, user_to_fd

	while (1) 
	{
		if ((ret = epoll_wait(epollfd, events, user_counter + 1, -1) < 0)
		{
			printf("epoll failure\n");
			break;
		}
		et(events, ret, epollfd, &user_counter, listenfd);
	}

}
