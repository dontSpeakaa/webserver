# webserver
## 采用epoll的方式来监听文件描述符

##将非监听的文件描述符注册为EPOLLONESHOT事件，确保操作系统最多触发其上注册的一个可读、
##可写或者异常事件，且只触发一次

##注册了EPOLLONESHOT事件的socket一旦被线程处理完成，应该重置该EPOLLONSHOT事件

##将线程池加入服务器，增加一个请求模块，将之前handleEvent的操作放到请求模块中，专门由请求模块调用线程执行。

##将服务器部分功能放入epoller中实现

##将定时器添加进服务器实现长连接，定时回调handler处理超时连接

##使用epoll与管道结合管理定时信号

##加入日志系统

##version_1
##加入线程池，减少创建线程所需的开销

##version_2
##加入对http请求的分析

##version_3
##加入定时器并通过epoll与管道结合来管理定时信号

##version_4
##加入日志系统
