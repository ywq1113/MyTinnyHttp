#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class sem
{
public:
    sem()
    {
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);
    }
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

class locker
{
public:
    locker()
    {
        if (pthread_mutex_init(&mutex_, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&mutex_);
    }
    bool lock()
    {
        return pthread_mutex_lock(&mutex_) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&mutex_) == 0;
    }
    pthread_mutex_t *get()
    {
        return &mutex_;
    }

private:
    pthread_mutex_t mutex_;

private:
    friend class cond;
};

class lockGuard
{
public:
    explicit lockGuard(locker &lock) : lock_(lock)
    {
        lock_.lock();
    }

    ~lockGuard()
    {
        lock_.unlock();
    }

private:
    locker &lock_;
};

class cond
{
public:
    cond(locker &lock) : lock_(lock)
    {
        if (pthread_cond_init(&cond_, NULL) != 0)
        {
            throw std::exception();
        }
    }

    ~cond()
    {
        pthread_cond_destroy(&cond_);
    }

    bool wait()
    {
        int ret = 0;
        ret = pthread_cond_wait(&cond_, lock_.get());
        return ret == 0;
    }

    bool timewait(struct timespec t)
    {
        int ret = 0;
        ret = pthread_cond_timedwait(&cond_, lock_.get(), &t);
        return ret == 0;
    }
    bool notify()
    {
        return pthread_cond_signal(&cond_) == 0;
    }
    bool notifyAll()
    {
        return pthread_cond_broadcast(&cond_) == 0;
    }

private:
    locker lock_;
    pthread_cond_t cond_;
};
#endif
