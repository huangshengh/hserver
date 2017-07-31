#include "log.h"
#include "hepoll.h"
#include "util.h"
extern int num;//数量
int count;

extern hconf t;
//工作线程来写,只需使用l的unixfd就可以了
void log_record(int fd,enum LOG_LEVEL level,const char* format,...)
{
    if(level <t.level)
        return;
    char buffer[logsize];//这个是用户输入的地方。
    char *p = buffer;
    char *s;

    if(level ==0)
    {
        s = "LOG_INFO";
        snprintf(buffer,10,"%s",s);
    }
    else if(level == 1)
    {
        s = "LOG_WARNING";
        snprintf(buffer,10,"%s",s);
    }
    else
    {
        s = "LOG_ERROR";
        snprintf(buffer,10,"%s",s);
    }
    p = buffer+strlen(buffer);
    *p++ = ' ';

    time_t t = time(NULL);
    struct tm* time = localtime(&t);
    if(strftime(p,100,"%c",time)==0)
        perror("timestamp failed: ");
    p = buffer +strlen(buffer);
    *p++ = ' ';

    va_list ap;
    va_start(ap,format);
    int n = logsize-strlen(buffer)-1;
    if(vsnprintf(p,n,format,ap)<0)
    {
        perror("contribute log buffer failed");
    }
    va_end(ap);

    //buffer 准备好了现在写。
    n= rio_writen(fd,buffer,strlen(buffer));
    if(n<0)
        perror("client write log file error");
}

int openfile()
{
    char filename[128] = "../log/";
    char* p;
    p = filename + strlen(filename);
    time_t t = time(NULL);
    struct tm* time = localtime(&t);
    snprintf(p,100,"log%d%d%d.txt",time->tm_sec,time->tm_min,time->tm_hour);
    int fd = open(filename,O_APPEND|O_CREAT|O_WRONLY,0666);
    if(fd <0)
    {
        perror("open log file failed: ");
        return -1;
    }
    return fd;
}

//log工作线程在写日志之前要申请缓冲区，4*logsize大小,每次写4096个字节
void log_main(Log l)
{
    int n,m;
    //已经满了
    if(l->remain == 0)
    {
        n = rio_writen(l->filefd,l->buffer,l->len);
        if(n<0)
            perror("write log to file error");
        l->remain = l->len;
        l->nread = 0;
        count += n;
    }
    if((n=read(l->unixfd,l->buffer,l->remain))>=0)
    {
        l->nread+= n;
        l->remain = l->len-l->nread;
        if(l->remain==0)
        {
            m = rio_writen(l->filefd,l->buffer,l->len);
            if(m<0)
                perror("write info to file error");
            l->nread = 0;
            l->remain = l->len;
            count += m;
        }
    }
    if((n <0)&&(errno != EINTR) )
        perror("read log failed: ");
    if(count>52428800)
    {
        count=0;
        l->filefd = openfile();
    }
}

void* loginit(void*f)
{
    int (*fd)[2];
    fd = f;
    Log l = (Log) malloc(sizeof(log));
    l->buffer = (char*)malloc(sizeof(char)*4*logsize);
    l->len = 4*logsize;
    l->remain= l->nread = 0;
    l->filefd = openfile();


    int epfd = hepoll_create(0);
    struct epoll_event ev;
    for(int i=0;i<=num;i++)
    {
        ev.data.fd = fd[i][1];//[0-num];0是主线程
        ev.events = EPOLLIN;//|EPOLLET;
        hepoll_add(epfd,fd[i][1],&ev);
        //makenoblock(fd[i][1]);
    }
    struct epoll_event* eventlist;//事件也不用太多
    eventlist = (struct epoll_event*)malloc(sizeof(struct epoll_event)*100);
    if(eventlist ==NULL)
    {
        perror("eventlist failed");
        return (void*)-1;
    }

    while(1)
    {
        int n = epoll_wait(epfd,eventlist,100,-1);
        for(int i=0;i<n;i++)
        {
            l->unixfd = eventlist[i].data.fd;
            log_main(l);
        }
    }
    free(l->buffer);
    free(l);
    return 0;
}
