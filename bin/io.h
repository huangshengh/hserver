#ifndef IO_H
#define IO_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>

#define MAXLINE 1024
#define readsize 256	

ssize_t readn(int fd,void *vptr,size_t n);
ssize_t writen(int fd,void *vptr,size_t n);

void Writen(int fd,void *ptr,size_t n);

//要添加readline函数，从connfd读入请求行。使用线程安全的readline
//使用线程特定的数据
static pthread_key_t r1_key;//进程中的Key结构数组,返回key（索引）
static pthread_once_t r1_once = PTHREAD_ONCE_INIT;//确保init参数指向的函数只调用一次

static void readline_destructor(void* ptr);

static void readline_once(void);

typedef struct{
	int r1_cnt;
	char* r1_bufptr;
	char r1_buf[MAXLINE];
}Rline;

//realine每次一个读缓冲区大小但是从缓冲区输出一个，缓冲区输完之后就再读一行。
static ssize_t my_read(Rline * tsd,int fd,char *ptr);

ssize_t readline(int fd,void * vptr,size_t maxlen);

#endif