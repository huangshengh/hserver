#define _GNU_SOURCE 
#include "http_parse.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "hepoll.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include "hepoll.h"
#include <sys/socket.h>
#include "log.h"
#include "util.h"
#include <sys/sendfile.h>


extern hconf t;

int transfer(struct request_t* t);
void http_parse(struct request_t* t);
//解析第一行和keep-alive选项
//从已连接的描述符上读第一行请求，然后解析request line
int http_read(struct request_t* re)
{
    int httpfd;
    httpfd = re->unixfd;
    int n;
    n = read(re->fd,re->buffer,BUFSIZE);
    if(n<=0)
    {
        if(errno != EINTR)
        {
            log_record(httpfd,0,"http_read.c close Connection fd:%d\n",re->fd);
            (*re->count)--;
            shutdown(re->fd,SHUT_RDWR);
            hepoll_del(re->epfd,re->fd,NULL);
            close(re->fd);
            free(re->buffer);
            free(re);
            return -1;
        }
    }
    re->size = strlen(re->buffer);
    http_parse(re);
    return 1;
}

void http_parse(struct request_t* re)
{
    int parsefd;
    parsefd = re->unixfd;
    char temp;
    for(int i=0;i<re->size;i++)
    {
        temp=re->buffer[i];
        if((temp=='\r')&&(i+1)<re->size)
        {
            re->buffer[i++] = '\0';
            re->buffer[i++] = '\0';
            re->requestline = re->buffer;
            re->header = re->buffer + i;
            break;
        }
    }
    if(re->requestline == NULL)
    {
        log_record(parsefd,2,"parse requestline wrong: %s\n",strerror(errno));
        return;
    }

    re->uri = strpbrk(re->requestline," \t");
    if(!re->uri)
    {
        log_record(parsefd,2,"parse uri wrong: %s\n",strerror(errno));
        return;
    }
    *re->uri++ = '\0';
    re->method = re->requestline;
    if(strcasecmp(re->method,"GET")!=0)
    {
        log_record(parsefd,2,"parse method wrong: %s\n",strerror(errno));
        return;
    }
    re->uri += strspn(re->uri," \t");
    re->version = strpbrk(re->uri," \t");
    if(re->version == NULL)
    {
        log_record(parsefd,2,"parse version wrong: %s\n",strerror(errno));
        return;
    }
    *re->version++ = '\0';
    re->version += strspn(re->version," \t");
    if(strstr(re->uri,".."))
    {
        log_record(parsefd,2,"prase.c  uri wrong: %s\n",strerror(errno));
        return;
    }
    if(strcasestr(re->header,"Connection: keep-alive"))
        re->keep_alive =1;
    else
        re->keep_alive = 0;
    transfer(re);
    /*
    struct epoll_event ev;
    ev.events = EPOLLOUT|EPOLLET|EPOLLONESHOT;
    ev.data.ptr = (void* )re;
    hepoll_mod(re->epfd,re->fd,&ev);
    */
    return;
}

int transfer(struct request_t* re)
{
    int transferfd;
    transferfd = re->unixfd;
    char *s = "close";
    char header[512];
    if(re->keep_alive)
        s = "Keep-Alive";
    int i,n;
    struct stat sbuf;
    char filepath[256];
    strcpy(filepath,t.rootfile);//"../decroot";
    if(!strcasecmp(re->uri,"/"))//默认情况
        strcat(filepath,"/index.html");
    else
        strcat(filepath,re->uri);
    if(stat(filepath,&sbuf)<0)//没有这个文件
        return -1;
    n=open(filepath,O_RDONLY);
    if(n<0)
    {
        log_record(transferfd,1,"no such file: %s\n",strerror(errno));
        snprintf(header,512,"HTTP/1.0 404 Not Found\r\nConnection: close\r\nServer: hserver\r\n\r\n");
        write(re->fd,header,strlen(header));
        (*re->count)--;
        hepoll_del(re->epfd,re->fd,NULL);
        shutdown(re->fd,SHUT_RDWR);
        close(re->fd);
        free(re->buffer);
        free(re);
        return -1;
    }
    else
    {
        struct epoll_event event;
        snprintf(header,512,"HTTP/1.1 200 OK\r\nServer: hserver\r\nContent-Length: %ld\r\nConnection: %s\r\nContent-Type: text/html\r\n\r\n",sbuf.st_size,s);
        write(re->fd,header,strlen(header));
       
        i = sendfile(re->fd,n,0,sbuf.st_size);
        close(n);
        if(i==-1)
        {
            log_record(transferfd,2,"transfer failed and close fd %s\n ",strerror(errno));
            hepoll_del(re->epfd,re->fd,NULL);
            (*re->count)--;
            shutdown(re->fd,SHUT_RDWR);
            close(re->fd);
            free(re->buffer);
            free(re);
            return 1;
        }
        
        if(re->keep_alive)
        {
            log_record(transferfd,0,"keep-alive Connection:%d \n",re->fd);
            event.events = EPOLLIN|EPOLLET|EPOLLONESHOT;//|EPOLLOUT;
            event.data.ptr= (void*)re;
            hepoll_mod(re->epfd,re->fd,&event);
            return 1;
        }
        (*re->count)--;
        hepoll_del(re->epfd,re->fd,&event);
        shutdown(re->fd,SHUT_RDWR);
        free(re->buffer);
        free(re);
        //close(re->fd);
        return 1;
    }
}

