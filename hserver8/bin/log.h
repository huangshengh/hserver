#ifndef HLOG_H
#define HLOG_H

#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define logsize 1024
enum LOG_LEVEL { LOG_INFO=0, LOG_WARNING, LOG_ERROR};

typedef struct 
{
    int unixfd;
    int filefd;//写文件的fd；
    //LOG_LEVEL level;
    char* buffer;//从套接字读缓冲区加入
    int nread;//已用
    int len;//总长度
    int remain;//还剩
}log,*Log;


void* loginit(void* f );
//客户线程的写
void log_record(int unixfd,enum LOG_LEVEL level,const char* format,...);

//日志线程的处理
void log_main(Log l);

#endif 