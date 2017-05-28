#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "io.h"

#define THREAD_NUM 4
#define MAXNCLI 32
pthread_mutex_t clifdmutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clifdcond=PTHREAD_COND_INITIALIZER;

typedef struct{
	int thread_count;
	pthread_t tid;
}threads;
threads* tpool;//tpool是线程池，用来管理线程的

int clifd[MAXNCLI];//存放描述符的
int iput;//就是clifd下一个存放的下标
int iget;//线程池中下一个获取描述符的下标。

void threadinit();//初始化线程，申请4个线程，有点少，但是cpu是4核的减少内核的切换
