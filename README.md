# HttpServer
Easy Http Server
## Resources
测试用资源
## Webbench
压力测试
## Exception
自定义异常类
## Lock
简化调用互斥锁与信号量 用该锁类实现线程间异步
## ThreadPoll
线程池
## Request
一个简要的HTTP请求类 主要负责解析Get HTTP请求和生成Get HTTP响应
## 踩坑
### 1.一开始想给每个请求类构造一个Map用来解析HTTP请求，结果Malloc溢出。没想到Map类这么消耗堆空间。
### 2.误关闭0号描述符，导致阻塞Accept返回-1，CPU主线程负载拉满。
### 3.std::move，本来想用移动语义减少使用开销，结果移动了一个Const，直接段错误。
### 4.最后的大坑，String在高并发的时候多个线程竟然可能会导致申请到相同的空间，真的没想到String竟然是这样的线程不安全，还以为只要把线程间访问的资源加锁即可。后续多线程处理直接加锁了String，就不跳段错误了。在6核虚拟机上跑到了1.7w左右的QPS，如果没加锁使用内置数组进行操作应该能更快，也有可能是HTML文件较小导致的QPS较高，能上一万的QPS还是比较满意的。
## 描述
主线程负责Accept请求，副线程负责通过Epoll监听读写以及给线程池注入任务，线程池中多个异步线程进行Http解析并生成Http响应，然后继续进行下一轮读写。
