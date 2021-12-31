#include "io.h"

int get_line(int sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;

	while ((i < size - 1) && (c != '\n'))
	{
		//recv()包含于<sys/socket.h>,参读《TLPI》P1259, 
		//读一个字节的数据存放在 c 中
		n = recv(sock, &c, 1, 0);
		/* DEBUG printf("%02X\n", c); */
		if (n > 0) 
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				/* DEBUG printf("%02X\n", c); */
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					c = '\n';
			}
			buf[i] = c;
			i++;
		}
		else
		c = '\n';
	}
	buf[i] = '\0';

	return(i);
}
