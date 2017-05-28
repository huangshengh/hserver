#include "log.h"
#include "io.h"

pthread_mutex_t logmutex = PTHREAD_MUTEX_INITIALIZER;

int openfile()
{
	int fd;
	if((fd= open("../log/log.txt",O_WRONLY|O_APPEND))<0)
	{
		perror("wrong open ");
		exit(0);
	}	
	return fd;
}

char* gettime()
{
	time_t t;
	struct tm *lt;
	char* strtime;
	time(&t);
	lt = localtime(&t);
	strtime = asctime(lt);
	return strtime;
}
void logerr(char* argv)
{
	char * stime;
	stime = gettime();
	char buf[1024];
	snprintf(buf,1024,"[error] (errno: %s) %s  %s",ERR(),argv,stime);
	char * fuck = buf;
	pthread_mutex_lock(&logmutex);
	int fd = openfile();
	Writen(fd,fuck,strlen(fuck));
	//fprintf(fd, "[error] (errno: %s) %s  %s", ERR(),argv,stime);//asctime返回的字符串自带\n
	close(fd);
	pthread_mutex_unlock(&logmutex);
}

//loginfo记录工作的消息
void loginfo(char*argv)
{
	char * stime = gettime();
	char buf[1024];
	snprintf(buf,1024,"[info]  %s   %s",argv,stime);
	char *fuck = buf;
	pthread_mutex_lock(&logmutex);
	int fd= openfile();
	Writen(fd,fuck,strlen(fuck));
	//fprintf(fd, "[error] (errno: %s) %s  %s", ERR(),argv,stime);//asctime返回的字符串自带\n
	close(fd);
	pthread_mutex_unlock(&logmutex);
}

void logsuccess(char *argv)
{
	char * stime = gettime();
	char buf[1024];
	snprintf(buf,1024,"[success]  %s   %s",argv,stime);
	char *fuck = buf;
	pthread_mutex_lock(&logmutex);
	int fd= openfile();
	Writen(fd,fuck,strlen(fuck));
	//fprintf(fd, "[error] (errno: %s) %s  %s", ERR(),argv,stime);//asctime返回的字符串自带\n
	close(fd);
	pthread_mutex_unlock(&logmutex);
}