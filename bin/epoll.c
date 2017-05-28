#include "epoll.h"
#include "log.h"

struct epoll_event *events;

int hepoll_create(int flags)
{
	int epfd;
	if((epfd = epoll_create1(flags))<=0)
	{
		logerr(" create epfd wrong");
		return -1;
	}
	events = (struct epoll_event*)malloc(sizeof(struct epoll_event)*MAXEVENTS);
	if(events==NULL)
	{
		logerr("malloc events wrong");
		return -1;
	}
	return epfd;
}

int hepoll_add(int epfd,int fd,struct epoll_event* ev)
{
	//ev传递过来的已经是修改之后的
	int n;
	if((n = epoll_ctl(epfd,EPOLL_CTL_ADD,fd,ev))!=0)
			logerr("add fd to epfd wrong");
	return n;
}

int hepoll_mod(int epfd,int fd,struct epoll_event* ev)
{
	int n;
	if((n = epoll_ctl(epfd,EPOLL_CTL_MOD,fd,ev))!=0)
		logerr("mod events on fd wrong");
	return n;
}

int hepoll_del(int epfd,int fd,struct epoll_event* ev)
{
	int n;
	if((n = epoll_ctl(epfd,EPOLL_CTL_DEL,fd,ev))!=0)
		logerr("delete fd wrong");
	return n;
}

int hepoll_wait(int epfd,struct epoll_event *ev,int maxevents,int timeout)
{
	int n;
	if((n=epoll_wait(epfd,ev,maxevents,timeout))<0)
		logerr("wait ready fd list wrong");
	return n;
}