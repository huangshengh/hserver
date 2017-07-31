#include "hepoll.h"
#include "log.h"

int hepoll_create(int flags)
{
    int epfd = epoll_create1(0);
    if(epfd<0)
    {
        //log
    }
    return epfd;
}

void hepoll_add(int epfd,int fd,struct epoll_event* ev)
{
    int rc = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, ev);
    if(rc == -1)
    //log();
    return;
}

void hepoll_mod(int epfd,int fd,struct epoll_event* ev)
{
    int rc = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, ev);
    if(rc == -1)
        //log
    return;
}

void hepoll_del(int epfd,int fd,struct epoll_event* ev)
{
    int rc = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, ev);
    if(rc == -1)
        //log
    return;
}

int hepoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) 
{
    int n = epoll_wait(epfd, events, maxevents, timeout);
    //if(n == -1)
        //log 
    return n;
}