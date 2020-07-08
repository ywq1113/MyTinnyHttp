#ifndef THREADDATA_H
#define THREADDATA_H

#include <unistd.h>
#include <sys/syscall.h>
#include <string>
#include <sys/prctl.h>
#include "thread.h"
#include "../base/currentThread.h"
#include "../base/CountDownLatch.h"

using namespace std;

/* 在头文件中声明，不要在头文件中定义，在.cpp文件中定义
namespace CurrentThread 
{
    t_cachedTid = 0;
    t_tidStringLength = 6;
    t_threadName = "default";
}
*/

class Thread;
struct threadData
{
    typedef std::function<void()> ThreadFunc;
    ThreadFunc func_;
    string name_;
    pid_t *tid_;
    CountDownLatch* latch_;
    threadData(const ThreadFunc& func, const string& name, pid_t *tid, CountDownLatch* latch)
      : func_(func), name_(name), tid_(tid), latch_(latch)
    {
    }

    void runInThread()
    {
        //通过指针修改tid的值，然后将指针置为0
        *tid_ = CurrentThread::tid();  //gdb调试在这一步Program received signal SIGABRT, Aborted.
        tid_ = NULL;
        latch_->countDown();
        latch_ = NULL;

        CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
        prctl(PR_SET_NAME, CurrentThread::t_threadName);  //设置线程名字
        func_();
        CurrentThread::t_threadName = "finished";
    }
};

#endif