## 采用epoll的方式来监听文件描述符

##将非监听的文件描述符注册为EPOLLONESHOT事件，确保操作系统最多触发其上注册的一个可读、
##可写或者异常事件，且只触发一次

##注册了EPOLLONESHOT事件的socket一旦被线程处理完成，应该重置该EPOLLONSHOT事件

##将线程池加入服务器，增加一个请求模块，将之前handleEvent的操作放到请求模块中，专门由请求模块调用线程执行。

##将服务器部分功能放入epoller中实现

##将定时器添加进服务器实现长连接，定时回调handler处理超时连接

##使用epoll与管道结合管理定时信号

##加入日志系统




##当static成员函数需要调用类的non-static成员对象时，可以将成员对象作为参数传入static函数

##当代码中使用了POSIX共享内存函数，编译的时候需要指定链接选项 -lrt

没有share文件夹时，在~目录输入：
sudo vmhgfs-fuse .host:/ /mnt/hgfs/ -o allow_other -o uid=1000

##nslookup   //允许运行该工具的主机查询任何指定的DNS服务器的DNS记录

##version_1
##加入线程池，减少创建线程所需的开销

##version_2
##加入对http请求的分析

##version_3
##加入定时器并通过epoll与管道结合来管理定时信号

##version_4
##加入日志系统

加入信号时出现问题，每一次从管道接受信号时，总是会出现系统中断，后来查看关于信号的内容，发现对于阻塞的系统调用，在
捕获到一个有效信号时，会使内核返回一个EINTR错误（被中断的系统调用），遇到这种情况，在添加信号时，给信号的flag设置
为SA_RESTART

在结束服务器进程时总会发生double free detected错误，经过查找发现在主进程结束时对线程池和epoll注册表重复调用delete

