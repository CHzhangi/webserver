#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "locker.h"
#include <condition_variable>
#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
using namespace std;

template <typename T>

class ThreadPool {
public:
    ThreadPool(int threadsCount = 8, int max_requests = 1000) : max_requests(max_requests), threadsCount(threadsCount), m_threads(NULL)
    {
        if (threadsCount <= 0 || max_requests <= 0) {
            throw std ::exception();
        }
        m_threads = new pthread_t[threadsCount];
        if (!m_threads)
            throw std ::exception();
        for (int i = 0; i < threadsCount; i++) {
            cout << "Creating thread " << i << endl;
            /*
            thread：一个指向 pthread_t 类型的指针，用于存储新线程的标识符。
            attr：一个指向 pthread_attr_t 类型的指针，用于指定新线程的属性，如线程栈大小、线程调度策略等。如果想使用默认属性，则可以将该参数设置为NULL。
            start_routine：一个指向函数的指针，该函数将作为新线程的入口函数，新线程将从该函数开始执行。该函数必须具有如下形式：void* (*start_routine)(
                void*)，即它接受一个 void* 类型的参数，返回一个 void* 类型的指针。
            arg：一个指向参数的指针，它将作为 start_routine函数的参数传递给新线程。如果不需要传递参数，则可以将该参数设置为 NULL。
            */
            if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
                delete[] m_threads;
                throw std ::exception();
            }

            if (pthread_detach(*(m_threads + i))) {
                delete[] m_threads;
                throw std::exception();
            }
        }
        cout << "线程创建完毕" << endl;
    }

    ~ThreadPool();

    bool addTask(T* task);

private:
    /*
    因为:start_routine：一个指向函数的指针，该函数将作为新线程的入口函数，新线程将从该函数开始执行。
    该函数必须具有如下形式：void* (*start_routine)(void*)，即它接受一个 void* 类型的参数，返回一个 void* 类型的指针。
    若线程函数为类成员函数，则this指针会作为默认的参数被传进函数中，从而和线程函数参数(void*)不能匹配，不能通过编译。
    静态成员函数就没有这个问题，里面没有this指针。
    */
    static void* worker(void* arg);
    void run();

private:
    //单例的线程池,不需要用static
    int threadsCount;
    int max_requests;
    pthread_t* m_threads;
    list<T*> workqueue;
    locker queue_locker;
    sem task_need_resolve;
    condition_variable cond;
    queue<T> tasks;
    // static void*       worker(void* arg);
};

/*ThreadPool::ThreadPool(size_t threadsCount)
{
    for (size_t i = 0; i < threadsCount; i++) {}
}*/
template <typename T>

ThreadPool<T>::~ThreadPool()
{
    delete[] m_threads;
}

template <typename T>

bool ThreadPool<T>::addTask(T* task)
{
    queue_locker.lock();
    if (workqueue.size() > max_requests) {
        queue_locker.unlock();
        return false;
    }
    workqueue.push_back(task);
    queue_locker.unlock();
    task_need_resolve.signal();
    return true;
}

template <typename T>

//由于worker是static的,所以不能直接使用成员变量,调用run函数
void* ThreadPool<T>::worker(void* arg)
{
    ThreadPool* pool = (ThreadPool*)arg;
    pool->run();
    return pool;
}

template <typename T>

void ThreadPool<T>::run()
{
    while (true) {
        task_need_resolve.wait();
        queue_locker.lock();
        if (workqueue.empty()) {
            queue_locker.unlock();
            break;
        }
        T* task = workqueue.front();
        workqueue.pop_front();
        queue_locker.unlock();
        if (!task) {
            continue;
        }
        task->process();
        // cout << "标记 before delete task" << endl;
        // cout << "标记 after delete task" << endl;
    }
}
#endif  // THREADPOOL_H