#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"
#include "http_conn.h"

template < typename T>
class threadpool{
public:
    threadpool(int thread_number=8, int max_request = 1000);
    ~threadpool();
    /*往请求队列中添加任务*/
    bool append(T* request);
private:
    /*工作线程运行的函数，从请求队列中取出任务并执行*/
    static void* worker(void* arg);
    void run();
    
    int m_thread_number;  //thread number in threadpool
    int m_max_request;  //the max number of request queue
    pthread_t* m_threads;  //the array of threads, size of it is m_thread_number
    std::list<T*> m_workqueue;  //request queue;
    locker m_queuelocker;  //the mutexlock to protect queue;
    sem m_queuestat;  //check whether is task to handle
    bool m_stop;  //whether stop the thread
};

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_request):
                m_thread_number(thread_number), m_max_request(max_request),
                m_stop(false), m_threads(NULL)
{
    if((thread_number<=0) || (max_request)<=0){
        throw std::exception();
    }
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads){
        throw std::exception();
    }
    
    /* create m_thread_number threads and set these threads as detach-threads*/
    for(int i=0;i<m_thread_number;++i){
        printf("create %d threads\n",i);
        if(pthread_create(m_threads+i, NULL, worker, this)!=0){
            delete[] m_threads;
            throw std::exception();
        }
        if(pthread_detach(m_threads[i])){
            delete[] m_threads;
            throw std::exception();
        }
    }
    
}

template<typename T>
threadpool<T>::~threadpool(){
    delete[] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T* request){
    /*操作工作队列时必须加锁，因为它被所有线程共享*/
    m_queuelocker.lock();
    if(m_workqueue.size() > m_max_request){
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}
    
template<typename T>
void* threadpool<T>::worker(void* arg){
    threadpool* pool = (threadpool*) arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run()
{
    while(!m_stop){
        m_queuestat.wait();
        m_queuelocker.lock();
        if(m_workqueue.empty()){
            m_queuelocker.unlock();
            continue;
        }
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if(!request)
            continue;
        request->http_conn::process();
    }
}
#endif

