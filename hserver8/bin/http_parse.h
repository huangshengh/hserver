#ifndef PARSE_H
#define PARSE_H

#define BUFSIZE 1024


struct request_t
{
    int thid;
    int unixfd;
    int* count;
    int epfd;
    int fd;
    char* buffer;
    char *header; 
    int size;
    char *method;
    char *version;
    char *uri;
    int keep_alive;
    char* requestline;
};

int http_read(struct request_t* re);
int transfer(struct request_t *re);

#endif