/*
* 这个服务器要有 处理http的状态，定时器，epoll，log，http传输，线程池。
* 我们这个主线程要做的就是创建线程和epoll，调用线程池
* 主线程使用管道和工作线程传递socket
*/
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <string.h>
#include <unistd.h>
#include "log.h"
#include "hepoll.h"
#include "util.h"
#include "threadpool.h"

#define MAXFD 65536

#define MAXLINE 4096
#define SA struct sockaddr

typedef void Sigfunc (int);

Sigfunc* signal(int signo,Sigfunc* func)
{
    struct sigaction act,oact;
    act.sa_handler =func;
    sigemptyset(&act.sa_mask);
    act.sa_flags =0;
    if(signo == SIGALRM)
    {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    }
    else
    {
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif
    }
    if(sigaction(signo,&act,&oact)<0)
        return SIG_ERR;
    return (oact.sa_handler);
}

hconf t; 
int  num;//= read_conf(confd);
int mfd;

int main(int argc, char *argv[])
{
    read_conf(&t);
    num = t.num;
    //忽略sigpipe信号
    signal(SIGPIPE,SIG_IGN);

    /*在这里读配置文件，读出来线程数量。现在假设为num
    配置文件里面就放一个线程数量。
    */
    //int  num = 2;//= read_conf(confd);
    int unixfd[num+1][2];//工作线程
    //主线程unixfd，工作线程都+1
    socketpair(AF_UNIX,SOCK_STREAM,0,unixfd[0]);
    mfd = unixfd[0][0];

    //创建num个管道,根据在线程池中的位置（index）来分配管道
    int tfd[num][2];
    for( int i = 0;i<num;i++)
    {
        if(pipe(tfd[i]) == -1)//每个线程使用它自己的管道。
        {
            log_record(mfd,2,"create pipe failed:%s\n",strerror(errno));
            exit(1);
        }
        socketpair(AF_UNIX,SOCK_STREAM,0,unixfd[i+1]);//每个线程有自己的unixfd。
    }
    pthread_t logt;
    //在这里创建logserver线程
    pthread_create(&logt,NULL,loginit,(void*)unixfd);

    //线程在这里创建.线程结构上有计数器,根据这个分发fd，不用锁。主线程只会读，每个线程写自己的
    hpool* pool = threadinit(num,tfd,unixfd);
    if(pool == NULL)
    {
        log_record(mfd,2,"threadpool initlized failed: %s\n",strerror(errno));
        exit(1);
    }


    int listenfd = socket(AF_INET,SOCK_STREAM,0);
    if(listenfd<0)
    {
        log_record(mfd,2,"create listenfd failed: %s\n",strerror(errno));
        return 1;
    }

    struct linger tmp = {1,0};
    setsockopt(listenfd,SOL_SOCKET,SO_LINGER,&tmp,sizeof(tmp));

    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY) ;
    address.sin_port = htons(t.port);

    if(bind(listenfd,(SA*)&address,sizeof(address))<0)
    {
        log_record(mfd,2,"bind address failed: %s\n",strerror(errno));
        return 1;
    }
	
    int a=1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&a,sizeof(int));

    
    if(listen(listenfd,50)<0)
    {
       log_record(mfd,2,"listen failed: %s\n",strerror(errno));
        return 1;
    }
    //setsockbuf(listenfd);
    //makenoblock(listenfd);

    //每个线程都有一个epoll，每个都有一个waitlist。
    struct epoll_event * events1;
    events1 = (struct epoll_event*)malloc(sizeof(struct epoll_event)* 2000);
    if(events1 == NULL)
    {
        log_record(mfd,2,"main thread malloc epoll_event list failed: %s\n",strerror(errno));
        close(listenfd);
        exit(1);
    }

    int epollfd = hepoll_create(0);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    hepoll_add(epollfd,listenfd,&ev);
    //主线程的epoll上面应该只有listenfd
    while(1)
    {
        int number = hepoll_wait(epollfd,events1,2000,-1);
        if((number<0)&&(errno!=EINTR))
        {
            log_record(mfd,1,"main thread epoll wait failed\n");
            break;
        }
        for(int i = 0; i<number;i++)
        {
            int infd = events1[i].data.fd;
            if(infd == listenfd)
            {
                int connfd;
                //因为一个listenfd上面可能有好几个已完成的连接，所以循环的连接
                while(1)
                {

                    struct sockaddr_in client_address;
                    socklen_t client_addrlen = sizeof(client_address);
                    connfd = accept(listenfd,(SA*)&client_address,&client_addrlen);
                    //setsockbuf(connfd);
                    if(connfd <0 )
                    {
                        if((errno == EAGAIN)||(errno == EWOULDBLOCK)||(errno == EINTR))
                            break;
                        else
                        {
                            log_record(mfd,2,"accept wrong\n");
                            break;
                        }
                    }
                    //主线程收到connfd之后，就用管道发送给线程。
                    int chose=0;//选择哪个线程分发fd。
                    int start = pool->arr[0].fdcount;
                    for(int i=0;i<num;i++)
                    {
                        if(pool->arr[i].fdcount<start)
                        {
                            chose = i;
                            start = pool->arr[i].fdcount;
                        }
                    }
                    //往写端开始写,线程会收到
                    if(write(tfd[chose][1],(void*)&connfd,sizeof(connfd))<0)
                    {
                        log_record(mfd,1,"send connfd to workthread wrong\n");
                        continue;
                    }
                }
            }
            else 
                close(infd);
        }
    }
    close(epollfd);
    close(listenfd);
    //资源要释放
    free(pool->arr);
    free(pool);
    return 1;//出现错误
}
