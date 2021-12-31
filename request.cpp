#include "request.h"

int startup(u_short *port)
{
	int httpd = 0;
	//sockaddr_in 是 IPV4的套接字地址结构。定义在<netinet/in.h>,参读《TLPI》P1202
	struct sockaddr_in name;

	//socket()用于创建一个用于 socket 的描述符，函数包含于<sys/socket.h>。参读《TLPI》P1153
	//这里的PF_INET其实是与 AF_INET同义，具体可以参读《TLPI》P946
	httpd = socket(PF_INET, SOCK_STREAM, 0);
	if (httpd == -1)
		my_error("socket");
       
	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	//htons()，ntohs() 和 htonl()包含于<arpa/inet.h>, 参读《TLPI》P1199
	//将*port 转换成以网络字节序表示的16位整数
	name.sin_port = htons(*port);
	//INADDR_ANY是一个 IPV4通配地址的常量，包含于<netinet/in.h>
	//大多实现都将其定义成了0.0.0.0 参读《TLPI》P1187
	name.sin_addr.s_addr = htonl(INADDR_ANY);
       
	//bind()用于绑定地址与 socket。参读《TLPI》P1153
	//如果传进去的sockaddr结构中的 sin_port 指定为0，这时系统会选择一个临时的端口号
	Bind(httpd, (struct sockaddr *)&name, sizeof(name));
              
	//如果调用 bind 后端口号仍然是0，则手动调用getsockname()获取端口号
	if (*port == 0)  /* if dynamically allocating a port */
	{
		int namelen = sizeof(name);
		//getsockname()包含于<sys/socker.h>中，参读《TLPI》P1263
		//调用getsockname()获取系统给 httpd 这个 socket 随机分配的端口号
		if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
			my_err("getsockname");
		*port = ntohs(name.sin_port);
	}
                                
	//最初的 BSD socket 实现中，backlog 的上限是5.参读《TLPI》P1156
	Listen(httpd, 5);
	
	return(httpd);
}


