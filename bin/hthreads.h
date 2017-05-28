#ifndef HTHR_H
#define HTHR_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define THREAD_NUM 8//线程的数量
#define MAXNCLI 64
pthread_mutex_t clifdmutex;
pthread_cond_t clifdcond;

typedef struct{
	int thread_count;
	pthread_t tid;
}threads;
threads* tpool;//tpool是线程池，用来管理线程的

int clifd[MAXNCLI];//存放描述符的
int iput;//就是clifd下一个存放的下标
int iget;//线程池中下一个获取描述符的下标。

void threadinit();//初始化线程，申请线程

#endif