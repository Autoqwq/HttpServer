#include "Lock.h"

//Lock类的构造函数
Lock::Lock(){
    //初始化互斥量
    int retMutexInit = pthread_mutex_init(&mutex,NULL);
    if(retMutexInit!=0){
        Error e("MutexInit Error!");
        throw std::exception(e);
    }
    //初始化信号量
    int retSemInit = sem_init(&sem,0,0); //线程可见 初值为0
    if(retSemInit==-1){
        Error e("SemInit Error!");
        throw std::exception(e);
    }
}

Lock::~Lock(){
    //销毁互斥量
    int retMutexDestroy = pthread_mutex_destroy(&mutex);
    //销毁信号量
    int retSemDestroy = sem_destroy(&sem); //线程可见 初值为0
}