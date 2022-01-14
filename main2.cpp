#include <unistd.h>
#include "server.h"

int main() {
	/* 守护进程 后台运行 */
	//daemon(1, 0); 

	Server server(
			9999, /* 端口 ET模式 timeoutMs 优雅退出  */
			3306, "root", "root", "webserver", /* Mysql配置 */
			12, 1024, 6, true, 1, 1024);             /* 连接池数量 number_of_client 线程池数量 日志开关 日志等级 日志异步队列容量 */
	server.start();
} 
