#include "hthreads.h"
#include "log.h"
#include "work.h"
void* threadwork(void * argv);

void threadinit()
{
	tpool = (threads*)malloc(sizeof(threads)*THREAD_NUM);
	for(int i=0;i<THREAD_NUM;i++)
	{
		int *iptr ;
		iptr = (int*) malloc(sizeof(int));
		*iptr = i;
		if(pthread_create(&tpool[i].tid,NULL,threadwork,(void*) iptr)!=0)//这个iptr没有什么用
		{
			logerr("pthread_create error");
			//exit(0);
			//错误处理,主线程立即返回，创建的线程会去处理work
		}
	}
}


void *threadwork(void * argv)
{
	int connfd;
	for(;;)
	{
		pthread_mutex_lock(&clifdmutex);
		while(iget == iput)//下一个要从clifd中获取的fd等于下一个主线程要存放的位置,就等待主线程先放
			pthread_cond_wait(&clifdcond,&clifdmutex);//两者相等无事可做，就阻塞，不相等就是，来的连接比较多，线程就不断的做
		connfd = clifd[iget];//从clifd获取要连接的描述符
		if(++iget==MAXNCLI)//iget更新位置
			iget=0;
		pthread_mutex_unlock(&clifdmutex);
		tpool[*((int*)argv)].thread_count++;//计算这个线程池中这个线程用了几次
		loginfo("hserver start work");
		httpin(connfd);//非阻塞的
		//close(connfd);失败的情况下closefd，成功的话放在clifd中。可以呆一会。
	}
}