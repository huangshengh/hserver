/*
* 线程池结构，有很多特性。
* 不用请求队列使用 管道来传递描述符。线程数组（这个是数组有pthread_t还有每个线程fd的count
），线程池的数目（这个从配置文件读）,
* 每个线程的线程函数，锁
*/
#ifndef POOL_H
#define POOL_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    int unixfd;
    pthread_t id;//每个线程的线程id。
    int fdcount;//每个线程都有一个fd计数器，主现程根据这个计数器来分配fd，实现负载均衡
    int mpipe;//每个线程的管道。fd[0]是读
}workthread;

typedef struct
{
    int threadnum;//线程的数目。主线程从conf文件中读取来，赋值。
    int index;//在线程池中这个线程的位置。
    //void* worker(void* t);//这个是我们工作线程运行的函数
    workthread* arr;//线程数组。    
} hpool;

//把每个线程的管道初始化
hpool* threadinit(int num,int(* mpip)[2],int(*unixfd)[2]);
void* threadwork(void* t);

#endif