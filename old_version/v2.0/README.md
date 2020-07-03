参考两猿社[https://github.com/qinguoyi/TinyWebServer](https://github.com/qinguoyi/TinyWebServer)

做了一些改动，但是整体架构没有变

* 单线程Reactor模型

* 异步日志

* 数据库连接池

* 非阻塞IO+Epoll多路复用+线程池处理连接

* 定时器处理非活动连接
