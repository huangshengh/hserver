#ifndef HEPOLL_H
#define HEPOLL_H

#include <sys/epoll.h>

#define MAX_EVENT_NUMBER 10000 

int hepoll_create(int flags);
void hepoll_add(int epfd,int fd,struct epoll_event* ev);
void hepoll_mod(int epfd,int fd,struct epoll_event* ev);
void hepoll_del(int epfd,int fd,struct epoll_event* ev);
//在线程中使用timeout定时。
int hepoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

#endif