#ifndef CURRENT_THREAD_H
#define CURRENT_THREAD_H

#include <stdint.h>

namespace CurrentThread 
{
    /*
     * __thread 是 GCC 内置的线程局部存储设施。
     * __thread 变量每一个线程有一份独立实体，各个线程的值互不干扰。
     * 可以用来修饰那些带有全局性且值可能变，但是又不值得用全局变量保护的变量。
     */
    extern __thread int t_cachedTid;
    extern __thread char t_tidString[32];
    extern __thread int t_tidStringLength;
    extern __thread const char* t_threadName;

    void cacheTid();  //缓存进程的tid

    inline int tid()
    {
        if(__builtin_expect(t_cachedTid == 0, 0)) {
            cacheTid();
        }
        return t_cachedTid;
    }

    inline const char* name() { return t_threadName; }
}

#endif