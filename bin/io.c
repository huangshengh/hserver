#include "io.h"

ssize_t readn(int fd,void *vptr,size_t n)
{
	size_t nleft;
	ssize_t nread;//ssize_t是有符号的
	char *ptr;//= (char*) malloc(MAXLINE);

	ptr = vptr;
	nleft = n;
	//要把输入的一直写完，除非碰见eof
	while(nleft>0)
	{
		if((nread = read(fd,ptr,nleft))<0)//出错了，eintr就再来读一次
		{
			if(errno ==EINTR)
				nread=0;
			else
				return -1;
		}
		else if(nread==0)//遇见了文件末尾，才会返回0
			break;
		nleft -= nread;
		ptr+=nread;
	}
	return (n-nleft);//有可能返回0,
}

ssize_t writen(int fd,void *vptr,size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char * ptr;
	ptr = vptr;
	nleft = n;
	while(nleft>0)
	{
		if((nwritten = write(fd,ptr,nleft))<=0)//成功返回写的字节数，错误是-1
			{
				if(errno ==EINTR)
					nwritten =0;
				else
					return -1;
			}
		nleft-= nwritten;
		ptr+=nwritten;
	}
	return n;//要么出错，要么n
}

void Writen(int fd,void *ptr,size_t n)
{
	ssize_t i;
	if(writen(fd,ptr,n)!=n)
		perror("write error");
}

static void readline_destructor(void* ptr)
{
	free(ptr);
}

static void readline_once(void)
{
	pthread_key_create(&r1_key,readline_destructor);//创建系统中Key数组中的键（索引），每个
	//线程可以随后为该键（索引）存储一个指针，这指针就是我们malloc 的数据的位置
}


//realine每次一个读缓冲区大小但是从缓冲区输出一个，缓冲区输完之后就再读一行。
static ssize_t my_read(Rline * tsd,int fd,char *ptr)
{
	if(tsd->r1_cnt<=0)//缓冲区读完了
	{
		again:
		if((tsd->r1_cnt = read(fd,tsd->r1_buf,readsize))<0)
		{
			if(errno == EINTR)
				goto again;
			//if(errno == EAGAIN||errno == EWOULDBLOCK)//没有数据
				//return 0;
			if(errno == EBADF)
				close(fd);
			return(-1);
		}
		else if(tsd->r1_cnt==0)//没有数据了
		{
			//close(fd);
			return 0;
		}
		tsd->r1_bufptr = tsd->r1_buf;
	}
	tsd->r1_cnt--;
	*ptr = *tsd->r1_bufptr++;
	return 1;
}

ssize_t readline(int fd,void * vptr,size_t maxlen)
{
	size_t n,rc;
	char c ,*ptr;
	Rline * tsd;

	pthread_once(&r1_once,readline_once);//r1_once确保readline_once只执行一次,在不同的
	//线程的pkey数组中找相同的索引，
	if((tsd = pthread_getspecific(r1_key))==NULL)
	{
		tsd = calloc(1,sizeof(Rline));
		pthread_setspecific(r1_key,tsd);
	}
	ptr = vptr;
	for(n=1;n<maxlen;n++)
	{
		if((rc = my_read(tsd,fd,&c))==1)
		{
			*ptr++ = c;
			if(c=='\n'||c=='\r')
				break;
		}
		else if(rc ==0)
		{
			*ptr =0;
			return n-1;//返回已读的数
		}
		else if(errno == EAGAIN||errno==EWOULDBLOCK)
			return n;//没有数据可读了
		else
			return -1;//返回-1，出现错误
	}
	*ptr = 0;
	//读完一行之后把缓冲清0
	bzero(tsd,sizeof(tsd));
	return n;//读了n个，最后一个是空字符.
}