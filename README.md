# hserver version1
A httpserver

## epoll + nonblockI/O + thread pool

使用一个全局的数组存放connfd，用条件变量来互斥，线程可以从数组中取出connfd去工作。
Epoll时间设置为ET和EPOLLONESHOT，只在状态改变的时候才通知，且当connfd工作完之后才从非激活太设置成激活太态。

目前还会出错，正在使用生产者消费者队列解决。

目前的只解析了http的请求行（使用线程特定数据保存），首部还没有解析。

# hserver version2 要实现:

实现首部部分字段的解析

实现timer

支持cgi