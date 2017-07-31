#include "threadpool.h"
#include "hepoll.h"
#include "util.h"
#include <unistd.h>
#include <errno.h>
#include "http_parse.h"
#include "log.h"
#include <sys/socket.h>
//那线程池的创建就先这样
extern int mfd;

hpool* threadinit(int num, int(* pip)[2],int (*unixfd)[2])
{
    if(num<=0)
    {
        log_record(mfd,2,"threadinit failed\n");
        return NULL;
    }

    hpool* pool;
    pool = (hpool*)malloc(sizeof(hpool));
    if(pool == NULL)
    {
        log_record(mfd,2,"malloc threadpool failed\n");
        return NULL;
    }
    pool->threadnum = num;
    pool->index =0;
    pool->arr = (workthread* )malloc(sizeof(workthread)*num);
    if(pool->arr == NULL)
    {
        log_record(mfd,2,"create attr failed\n");
        return NULL;
    }
    //初始化每个线程结构的信息。
    for(int i=0;i<num;i++)
    {
        pool->arr[i].unixfd = unixfd[i+1][0];
        pool->arr[i].mpipe = pip[i][0];
        pool->arr[i].fdcount = 1;
    }
    //pool->worker = threadwork;

    pthread_attr_t attr;//利用线程的分离属性。
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    for(int i=0;i<num;i++)
    {
        if(pthread_create(&(pool->arr[i].id),&attr,threadwork,(void *)(&pool->arr[i]))!=0)
        {
            log_record(mfd,2,"pthread_create failed: %s\n",strerror(errno));
            free(pool->arr);
            free(pool);
            return NULL;
        }
    }
    return pool;
}

//线程的工作函数创建epoll，调用http入口函数，线程里面的connfd就要使用结构体来ptr。
void* threadwork(void *t)
{
    int thunixfd;
    int tpoll = epoll_create1(0);

    workthread * th = (workthread*)t;
    th->id = pthread_self();
    thunixfd = th->unixfd;
    int pipefd = th->mpipe;//pipe[0];
    struct epoll_event event;
    event.events = EPOLLIN;//|EPOLLET; //先用data。fd，可能之后再加入struct
    event.data.fd = pipefd;

    hepoll_add(tpoll,pipefd,&event);
    //makenoblock(pipefd);

    struct epoll_event * tevents;
    tevents = (struct epoll_event*) malloc(sizeof(struct epoll_event)*2000);

    for(;;)
    {
        //定时器待会加，现在先用-1
        int tn = hepoll_wait(tpoll,tevents,2000,-1);
        if((tn <0)&&(errno != EINTR))
        {
            log_record(thunixfd,1,"workthread %d epoll wait failed: %s\n",th->id,strerror(errno));
            perror("workthread epoll wait error");
            continue;
        }
        for(int ti=0;ti<tn;ti++)
        {
            int tsockfd = tevents[ti].data.fd;
            //说明是新的连接描述符到来。
            if(tsockfd == pipefd)
            {
                int tconnfd;
                int tr = read(tsockfd,(void*)&tconnfd,sizeof(int));
                if(tr<0)
                {
                    log_record(thunixfd,1,"workthread %d read sockfd from pipe error:%s\n",th->id,strerror(errno));
                    continue;
                }
                if(tconnfd<0)
                    continue;
                th->fdcount++;
                //把connfd添加到epoll中。
                struct request_t *re = (struct request_t*)malloc(sizeof(struct request_t));
                re->thid = th->id;
                re->unixfd = th->unixfd;
                re->fd = tconnfd;
                re->epfd = tpoll;
                re->count = &th->fdcount;
                re->buffer = (char*)malloc(sizeof(char)*BUFSIZE);

                struct linger l;
                l.l_onoff=1;
                l.l_linger=0;
                setsockopt(tconnfd,SOL_SOCKET,SO_LINGER,&l,sizeof(struct linger));
                struct epoll_event event;
                event.events = EPOLLIN|EPOLLET|EPOLLONESHOT;
                event.data.ptr = re;
                hepoll_add(tpoll,tconnfd,&event);
                makenoblock(tconnfd);
            }
            else if(tevents[ti].events & EPOLLIN)//connfd的请求
            {
                http_read((struct request_t*)tevents[ti].data.ptr);
            }
            else if(tevents[ti].events & EPOLLOUT)//上次没有写完
            {
                transfer((struct request_t*)tevents[ti].data.ptr);
            }
            else
            {
                log_record(thunixfd,1,"workthread %d come some other message\n",th->id);
                close(((struct request_t*)tevents[ti].data.ptr)->fd);
                free(tevents[ti].data.ptr);
                //shutdown(tevents[ti].data.fd,SHUT_RDWR);
                th->fdcount--;
            }
        }
    }
}


