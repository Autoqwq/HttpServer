#ifndef THREAD_POLL_H
#define THREAD_POLL_H

#include "../Exception/Exception.h"
#include "../Lock/Lock.h"
#include "../Request/Request.h"

#include "pthread.h"
#include <deque>

class ThreadPoll{
public:
    ThreadPoll(int threadCount,int maxRequest);
    ~ThreadPoll();


    //给线程池追加任务
    bool ThreadAppend(Request* req);
private:


    //线程被创造后会作为入口函数
    static void* ThreadStart(void* arg);

    //线程实际不断做事情的函数
    void ThreadRun();

    //判断线程池是否开启
    bool threadOpen;

    //线程池的锁
    Lock *lock;

    //任务队列
    std::deque<Request*> works;

    //线程池的最大请求数,追加请求时判断,如果超过这个数量则追加失败。
    int maxRequest;

    //线程池的线程
    pthread_t *threadPool;

    //线程池的线程数
    int threadCount;


};


#endif