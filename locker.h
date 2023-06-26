#ifndef LOCKER_H
#define LOCKER_H
#include <exception>
#include <pthread.h>
#include <semaphore.h>
class locker {
public:
    locker()  //构造locker类，调用pthread_mutex_init函数初始化
    {
        if (pthread_mutex_init(&mutex, NULL) != 0) {
            throw std::exception();
        }
    }

    ~locker()
    {  //析构函数
        pthread_mutex_destroy(&mutex);
    }

    bool lock()
    {  //上锁，调用pthread_mutex_lock函数
        return pthread_mutex_lock(&mutex) == 0;
    }

    bool unlock()
    {  //解锁，调用pthread_mutex_unlock函数
        return pthread_mutex_unlock(&mutex) == 0;
    }

private:
    pthread_mutex_t mutex;
};

class cond {
public:
    cond()
    {
        if (pthread_cond_init(&m_cond, NULL) != 0) {
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }

    bool wait(pthread_mutex_t* mutex)
    {
        return pthread_cond_wait(&m_cond, mutex) == 0;
    }

    bool timewait(pthread_mutex_t* m_mutex, struct timespec t)
    {
        int ret = 0;
        // pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        // pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    pthread_cond_t m_cond;
};

class sem {
public:
    sem()
    {
        if (sem_init(&m_sem, 0, 0) != 0) {
            throw std::exception();
        }
    }

    sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0) {
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

    bool signal()
    {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};
#endif  // LOCKER_H