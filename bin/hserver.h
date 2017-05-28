#ifndef HSERVER_H
#define HSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include "log.h"
#define MAX_EVENTS 5; 



typedef struct sockaddr SA;
//typedef void Signal(int);

int Socket(int family,int type,int protocol)
{
	int n;
	if((n=socket(family,type,protocol))<0)
		logerr("socket error");
	return n;
}

int Bind(int sockfd,SA* addr,socklen_t addrlen)
{
	int n;
	if((n = bind(sockfd,addr,addrlen))<0)
		logerr("bind error");
	return n;
}


int Listen(int fd,int blocklog)
{
	int n;
	if((n = listen(fd,blocklog))<0)
		{
			logerr("listen error");
			exit(0);
		}
	return n;
}

//把描述符设置成非阻塞
int setnonblock(int fd)
{
	int flags;
	if((flags=fcntl(fd,F_GETFL,0))<0)
		logerr("get fd property wrong");
	flags |= O_NONBLOCK;
	if(fcntl(fd,F_SETFL,flags)<0)
		logerr("set fd noblocking wrong");
	return fd;
}

#endif