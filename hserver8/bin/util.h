#ifndef UTIL_H
#define UTIL_H

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct{
    int port;
    int num;
    char rootfile[256];
    int level;
}hconf,*phconf;


void makenoblock(int fd);

//在这里读取配置文件
void read_conf(phconf t);
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
void setsockbuf(int sockfd);
#endif
