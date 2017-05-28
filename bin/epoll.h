#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#define MAXEVENTS 128

int hepoll_create(int size);
int hepoll_add(int epfd,int fd,struct epoll_event* ev);
int hepoll_mod(int epfd,int fd,struct epoll_event* ev);
int hepoll_del(int epfd,int fd,struct epoll_event* ev);
int hepoll_wait(int epfd,struct epoll_event *ev,int maxevents,int timeout);

#endif