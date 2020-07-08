#ifndef COUNTDOWNLATCH_H
#define COUNTDOWNLATCH_H
#include "../lock/locker.h"

class CountDownLatch : noncopyable 
{
public:
    explicit CountDownLatch(int count);
    void wait();
    void countDown();

private:
    mutable locker lock_;
    cond cond_;
    int count_;  //count_初始值为线程数量
};

#endif