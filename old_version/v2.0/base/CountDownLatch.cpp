#include "CountDownLatch.h"
#include "../lock/locker.h"

CountDownLatch::CountDownLatch(int count)
    : lock_(), cond_(lock_), count_(count) {}

void CountDownLatch::wait() {
    lockGuard lg(lock_);
    while(count_ > 0) cond_.wait();
}

void CountDownLatch::countDown() {
    lockGuard lg(lock_);
    count_--;
    if(count_ == 0) cond_.notifyAll();
}