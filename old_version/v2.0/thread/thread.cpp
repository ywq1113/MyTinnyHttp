#include "thread.h"
#include "../base/CountDownLatch.h"

Thread::Thread(const ThreadFunc& func, const std::string& name)
    : started_(false),
      joined_(false),
      pthreadId_(0),
      tid_(0),
      func_(func),
      name_(name),
      latch_(1)
{
    if(name_.empty())
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "Thread");
        name_ = buf;
    }
}

Thread::~Thread()
{
    // 如果线程已经启动并且线程不需要阻塞关闭别的线程
    // 析构时将其设为分离状态，线程会自动关闭
    if(started_ && !joined_) {
        pthread_detach(pthreadId_);
    }
}

//::syscall 间接系统调用
pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

namespace CurrentThread {
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char* t_threadName = "default";
}

void CurrentThread::cacheTid()
{
    if(t_cachedTid == 0) 
    {
        t_cachedTid = gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), 
                                     "%5d ", t_cachedTid);
    }
}

// 创建线程时调用的函数
void* startThread(void* obj)
{
    threadData *data = static_cast<threadData*>(obj);
    data->runInThread();
    delete data;
    return NULL;
}

void Thread::start()
{
    assert(!started_);
    started_ = true;
    threadData* data = new threadData(func_, name_, &tid_, &latch_);
    if(pthread_create(&pthreadId_, NULL, &startThread, data /*运行函数的参数*/) /*返回非0值为出错*/)
    {
        started_ = false;
        delete data;
    }
    else
    {
        latch_.wait();
        assert(tid_ > 0);
    }
}

//负责释放别的线程
int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}