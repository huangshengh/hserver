#ifndef work_h
#define work_h

#include "httpparse.h"
#include "log.h"
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

pthread_mutex_t workmutex;

int transfer(int fd,char *uri);
int httpin(int fd);

#endif