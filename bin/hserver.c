#include "hserver.h"
#include "hthreads.h"
#include "epoll.h"

//epoll最多关注128个，clifd放64个描述符
//在处理函数中从不close fd

extern struct epoll_event* events;//在epoll.c中申请了空间
pthread_mutex_t clifdmutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clifdcond=PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[])
{
	int listenfd,connfd,n;
	struct sockaddr_in cliaddr,serveraddr;
	socklen_t clilen;
	pthread_t tid;

	threadinit();//创建了thread_num个线程的线程池tpool每个线程都阻塞在clifdmutex处

	listenfd = socket(AF_INET,SOCK_STREAM,0);

	bzero(&serveraddr,sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(12000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	Bind(listenfd,(SA*)&serveraddr,sizeof(serveraddr));
	Listen(listenfd,10);//调用了epoll但是有可能一下子来多个客户端的连接。
	listenfd = setnonblock(listenfd);

	//处理sigpipe信号，默认会终止进程
	struct sigaction sa;
	bzero(&sa,sizeof(sa));
	sa.sa_handler= SIG_IGN;//hulve
	sa.sa_flags =0;
	if (sigaction(SIGPIPE, &sa, NULL)) {
        logerr("install sigal handler for SIGPIPE failed");
        return 0;
    }

	//加入epoll的是listenfd和每次连接的connfd，
	int epfd;
	epfd= hepoll_create(0);
	struct epoll_event event;
	event.events = EPOLLIN|EPOLLET;
	event.data.fd = listenfd;
	hepoll_add(epfd,listenfd,&event);
	for(;;)
	{
		//events,我们已经申请过空间了
		n = hepoll_wait(epfd,events,MAXEVENTS,-1);//如果同时来的信号的话，那么我们会轮询的。
		for(int i=0;i<n;i++)
		{
			int infd;
			infd = events[i].data.fd;
			if(infd == listenfd)
			{
				while(1)
				{
				connfd = accept(listenfd,(SA*)&cliaddr,&clilen);
				loginfo("accepted ");
				if(connfd<0)
				{
					if(errno==EAGAIN||errno ==EWOULDBLOCK)
						break;
					else
					{
						logerr("accept wrong");
						break;
					}
				}
				//connfd来了，我们这个添加进生产者消费者队列
				pthread_mutex_lock(&clifdmutex);
				connfd = setnonblock(connfd);
				event.events = EPOLLIN|EPOLLET|EPOLLONESHOT;//就是在connfd上的操作没有完成之前connfd上再来数据我们也不返回
				event.data.fd = connfd;
				hepoll_add(epfd,connfd,&event);
				loginfo("server epoll add a connfd to interest list");
				clifd[iput]= connfd;//添加到clifd,clifd最多有128个，但是工作的线程只有8个
				if(++iput == MAXNCLI)//64,从头开始,把前面的clifd里面的fd换掉
					iput=0;
				//input的connd的时候如果前面有数据，说明已经循环了clifd，我们把前面的fd删掉，并且epfd中也不再关注
				int i = iput+1;
				if(clifd[i]!=0)
				{
					hepoll_del(epfd,clifd[i],NULL);//所以我们每个fd都会alive一个数组轮回的时间
					close(clifd[i]);
				}
				if(iput==iget)//我们input比get大了一圈，因为get是使用信号增加的，所以get不会跑到input前面
				{
					//线程处理的比较慢
					sleep(2);//睡一会。
				}
				pthread_cond_signal(&clifdcond);//发送信号
				pthread_mutex_unlock(&clifdmutex);//然后解锁，线程就去为客户服务，我们主线程就去做自己的事情
			}//end of while accept
			}
			//else我们直接来操作，说明这是一个已连接的描述符。
			else 
			{
				 if ((events[i].events & EPOLLERR) ||(events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN))) //这些fd都不用处理
				{
                    logerr("epoll error fd");
                    hepoll_del(epfd,infd,NULL);
                    close(infd);
                    continue;
                }
                loginfo("new data came");
                //infd是clifd里面的描述符。
                //还是放在clifd里面等着线程取吧
                pthread_mutex_lock(&clifdmutex);
                clifd[iput]= infd;
                if(++iput == MAXNCLI)//62从头开始,把前面的clifd里面的fd换掉
					iput=0;
					//hepoll_del(epfd,clifd[iput],NULL);
					//close(clifd[iput]);
				//input的connd的时候如果前面有数据，说明已经循环了clifd，我们把前面的fd删掉，并且epfd中也不再关注
				int a = iput+1;
				if(clifd[a]!=0&&infd!=clifd[a])//因为这之前的fd可能已经关闭了
				{
					hepoll_del(epfd,clifd[a],NULL);//所以我们每个fd都会alive一个数组轮回的时间
					close(clifd[a]);
				}
				if(iput==iget)//我们input比get大了一圈，因为get是使用信号增加的，所以get不会跑到input前面
				{
					//线程处理的比较慢
					sleep(2);//睡一会。
				}
				pthread_cond_signal(&clifdcond);//发送信号
				pthread_mutex_unlock(&clifdmutex);
			}
		}
	}
	return 0;
}
