#include "ThreadPoll.h"



ThreadPoll::ThreadPoll(int threadCount,int maxRequest){
    //初始化线程数量和最大请求数
    if(threadCount<=0||maxRequest<=0){
        Error e("threadCount or maxRequest can't be negetive!");
        throw e;
    }
    this->threadCount=threadCount;
    this->maxRequest=maxRequest;

    lock = new Lock();  //初始化锁

    threadOpen=true;    //开启线程池


    //构造线程数组
    threadPool = new pthread_t[threadCount];
    for(int i=0;i<threadCount;++i){
        //构造线程
        int retPthreadCreat = pthread_create(threadPool+i,NULL,ThreadStart,this);
        if(retPthreadCreat==-1){
            Error e=Error("PthreadCreat Error!");
            throw e;
        }
        
        //设置线程分离
        int retPthreadDetach=pthread_detach(*(threadPool+i));
        if(retPthreadDetach==-1){
            Error e=Error("PthreadDetach Error!");
            throw e;
        }
    }
    
}


bool ThreadPoll::ThreadAppend(Request* req){
    if(works.size()>maxRequest){
        return false;
    }

    //上锁
    lock->MutexLock();
    //添加任务
    works.push_back(req);
    lock->SemPost();
    //解锁
    lock->MutexUnlock();

    return true;
}



void* ThreadPoll::ThreadStart(void* arg){

    //获取线程池指针准备回调
    ThreadPoll* threadPool=(ThreadPoll*)(arg);

    //回调
    threadPool->ThreadRun();

    return NULL;

}


void ThreadPoll::ThreadRun(){
    //如果线程池开启则准备做任务
    while(threadOpen){
        //等待任务
        lock->SemWait();
        //上锁
        lock->MutexLock();
        //获取该次连接请求
        Request* req =works.front();
        works.pop_front();
         //解锁
        lock->MutexUnlock();
        //处理请求
        req->Process();
    }
}

    
ThreadPoll::~ThreadPoll(){
    //关闭线程池开关
    threadOpen=false;

    //清除堆空间
    delete lock;
    delete[] threadPool;
}