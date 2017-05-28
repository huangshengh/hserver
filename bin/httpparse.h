//先来写试试只要解析第一行

#ifndef httpparse_h
#define httpparse_h

#include <stdio.h>//sscanf
#include <stdlib.h>
#include "io.h"

int ok_respond();

int err_respond(int fd);


#endif