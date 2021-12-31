int setnonblocking(int fd);

void addfd(int epollfd, int fd, bool enable_et);

void reset_oneshot(int epollfd, int fd);

int get_dest_and_message(int fd, char* buf, char* dest);

void point_message();

void et();

int epoll();

