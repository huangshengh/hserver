# hserver

## hserver is a simple httpserver with threadpool and epoll. 
This version has many bugs

## hserver8  is a stronger version. 
### Model
main thread hold listenfd in epoll,each work thread also has epoll. Main thread use pipe to communicate with work thread,so we can
tranfer the connfd.

### Epoll
use epollet, epolloneshot.

### http parse
Parse the header to achieve keep-alive functions.

### log
Use UNIX socket to communicate log server thread and log client thread. This works bad,use linkedlist with lock may be better.

