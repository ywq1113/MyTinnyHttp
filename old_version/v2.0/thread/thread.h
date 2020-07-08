#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <assert.h>
#include <functional>
#include <memory>
#include <string>
#include "../base/CountDownLatch.h"
#include "../base/noncopyable.h"
#include "threadData.h"

struct threadData;

class Thread : noncopyable {
public:
    typedef std::function<void()> ThreadFunc;
    explicit Thread(const ThreadFunc&, const std::string&);
    ~Thread();
    void start();
    int join();
    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

private:
    void setDefaultName();
    bool started_;
    bool joined_;
    pthread_t pthreadId_;
    pid_t tid_;  //持有线程所在的进程号
    ThreadFunc func_;
    std::string name_;
    CountDownLatch latch_;
};

#endif