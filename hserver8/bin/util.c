#include "util.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

void read_conf(phconf t)
{
    FILE* confp = fopen("hconf.txt","r");
    char buffer[1024];
    char po[20];
    char th[20];
    char root[20];
    char rootfile[256];
    char port[20];
    char num[20];
    char level[20];
    char log[20];
    //fgets每次读入一行，包括换行符。最多能放1023个正常字符，最后一个是它自己的null
    fgets(buffer,1024,confp);
    sscanf(buffer,"%s = %s",po,port);
    fgets(buffer,1024,confp);
    sscanf(buffer,"%s = %s",th,num);
    fgets(buffer,1024,confp);
    sscanf(buffer,"%s = %s",root,rootfile);
    fgets(buffer,1024,confp);
    sscanf(buffer,"%s = %s",log,level);
    t->level = atoi(level);
    t->port = atoi(port);
    t->num = atoi(num);
    strcpy(t->rootfile,rootfile);
}

void makenoblock(int fd)
{
    int m = fcntl(fd,F_GETFL,0);
    fcntl(fd,F_SETFL,m|O_NONBLOCK);
}

ssize_t rio_readn(int fd, void *usrbuf, size_t n) 
{
        size_t nleft = n;
        ssize_t nread;
        char *bufp = (char *)usrbuf;

         while (nleft > 0) {
            if ((nread = read(fd, bufp, nleft)) < 0) {
             /* interrupted by sig handler return */
            if (errno == EINTR) 
                nread = 0;      /* and call read() again */
            else
                return -1;      /* errno set by read() */ 
                                                        
            } else if (nread == 0)
                break;              /* EOF */
            nleft -= nread;
            bufp += nread;
                                        
        }
        return (n - nleft);         /* return >= 0 */

}

ssize_t rio_writen(int fd, void *usrbuf, size_t n) 
{
        size_t nleft = n;
        ssize_t nwritten;
        char *bufp = (char *)usrbuf;
                        
        while (nleft > 0) {
            if ((nwritten = write(fd, bufp, nleft)) <= 0) {
                if (errno == EINTR)  /* interrupted by sig handler return */
                    nwritten = 0;    /* and call write() again */
                else 
                    return -1;       /* errorno set by write() */
                                                        
            }
            nleft -= nwritten;
            bufp += nwritten;
                                        
        }
        return (n - nleft);
}

void setsockbuf(int sockfd)
{
    int recv=614400,send=614400;
    setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&recv,sizeof(int));
    setsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,&send,sizeof(int));
    //printf("re %d se %d\n",recv,send);
}
