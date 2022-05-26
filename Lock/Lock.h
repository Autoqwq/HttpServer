#ifndef LOCK_H
#define LOCK_H

#include <pthread.h>
#include <semaphore.h>
#include "../Exception/Exception.h"

//实现互斥量和信号量
class Lock{
public:
    Lock();
    ~Lock();

    //互斥量的加锁和解锁
    inline int MutexLock(){
        return pthread_mutex_lock(&mutex);
    }
    inline int MutexUnlock(){
        return pthread_mutex_unlock(&mutex);
    }

    //信号量的处理函数
    inline int SemPost(){
        return sem_post(&sem);
    }
    inline int SemWait(){
        return sem_wait(&sem);
    }
private:

pthread_mutex_t mutex;  //互斥锁 防止多个线程访问线程池
sem_t sem;              //信号量 为当前的可做人物值

};


#endif