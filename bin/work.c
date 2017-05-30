#include "work.h"
//http有关的操作
#include "epoll.h"

pthread_mutex_t workmutex=PTHREAD_MUTEX_INITIALIZER;

int transfer(int fd,char *uri);
//从已连接的描述符上读第一行请求，然后解析request line
int httpin(int epfd,int fd)
{
	int n;
	char ptr[readsize];
	char method[32];
	char uri[128];
	char version[32];
	//只读取第一行。request。
	//我们的fd是非阻塞的会立马返回，不会等
	struct epoll_event eve;
	eve.data.fd = fd;
	eve.events = EPOLLIN|EPOLLET|EPOLLONESHOT;
	if((n = readline(fd,ptr,readsize))==-1)
	{
		//printf("requestline: %s\n", ptr);
		logerr("readline from connfd wrong");
		//close(fd);
		hepoll_mod(epfd,fd,&eve);
		return -1;
	}
	if(n==0)
		return 0;
	//解析第一行
	sscanf(ptr,"%s %s %s",method,uri,version);
	//如果是get
	if(!strcasecmp(method,"GET"))
	{
		int i;
		if((i = transfer(fd,uri))==1)
		{
			hepoll_mod(epfd,fd,&eve);
			return 1;
		}
		else
		{
			hepoll_mod(epfd,fd,&eve);
			return -1;
		}
	}
	else
	{
		err_respond(fd);
		//close(fd);
		hepoll_mod(epfd,fd,&eve);
		return -1;
	}
}

//1是正确传输，-1就是失败
//get就传输，close fd在线程最后就会close
//把文件传给fd
int transfer(int fd,char *uri)
{
	char ptr[512];
	strcpy(ptr,uri);
	void * start;
	struct stat sbuf;//这个得到文件的大小信息，可以在映射的时候使用
	int n,i;
	char filepath[514] = "..";
	if(!strcasecmp(ptr,"/"))//默认情况
	{
		char temp [128] = "../decroot/index.html";
		strcpy(filepath,temp);
	}
	else
		strcat(filepath,ptr);
	if(stat(filepath,&sbuf)<0)//没有这个文件
	{
		logerr("filepath is wrong");
		//perror("stat error");
		err_respond(fd);
		//close(fd);
		return -1;
	}
	n=open(filepath,O_RDONLY);
	if(n<0)
	{
		logerr("open file failed");
		err_respond(fd);
		//close(fd);
		return -1;
	}
	else//传输文件了。
	{
		start = mmap(NULL,sbuf.st_size,PROT_READ,MAP_PRIVATE,n,0);
		if(start == (void*) -1)
		{
			logerr("mmap failed");
			//close(fd);
			return -1;
		}
		close(n);//关闭文件描述符
		if((i=writen(fd,start,sbuf.st_size))==-1)//传输文件。
		{
			logerr("write failed");
			//close(fd);
			return -1;
		}
		logsuccess("transfer success");
		munmap(start,sbuf.st_size);
		return 1;
	}
}

