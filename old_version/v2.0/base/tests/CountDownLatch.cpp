#include "CountDownLatch.h"

CountDownLatch::CountDownLatch(int count)
    : lock_(), condition_(lock_), count_(count) {}

void CountDownLatch::wait() {
    lockGuard lg(lock_);
    while(count > 0) cond_.wait();
}

void CountDownLatch::countDown() {
    lockGuard lg(lock_);
    count_--;
    if(count_ == 0) cond_.notifyAll();
}