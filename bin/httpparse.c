#include "httpparse.h"
int err_respond(int fd)
{
	char buf[MAXLINE];
	snprintf(buf,MAXLINE,"HTTP 405 DON'TSUPPORT\r\n");
	writen(fd,buf,strlen(buf));
	return 0;
}