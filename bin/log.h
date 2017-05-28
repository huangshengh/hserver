#ifndef LOG_H
#define LOG_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

//使用自己的互斥锁。log锁
pthread_mutex_t logmutex;
//多线程下写日志，我们使用fd
#define ERR() (errno==0?"NONE":strerror(errno))	
int openfile();

char* gettime();

//logerr记录错误的消息,考虑多线程的情况

void logerr(char* argv);

//loginfo记录工作的消息
void loginfo(char*argv);

void logsuccess(char *argv);

#endif