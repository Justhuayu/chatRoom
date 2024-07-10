#pragma once
#define THREAD_NUMBER_FACTOR 2

#include <iostream>
#include <pthread.h>
#include <list>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <memory>

template <typename T>
class threadPool
{
private:
    /* data */
public:
    threadPool();
    ~threadPool();
    void append(std::shared_ptr<T> arg);//向工作队列中添加
    
private:
    static void* worker(void* arg);//线程入口函数，static是__cdecl约定，而类成员函数是__thiscall约定
    void run();//执行函数
private:
    size_t m_thread_numbers;//线程池线程数
    pthread_t *m_threads;//指向线程id的数组
    std::list<std::shared_ptr<T>> m_work_queue;//工作队列
    std::mutex m_mutex;//工作队列的读写锁
    std::condition_variable m_condition;//工作队列的条件变量
    bool m_stop;//停止线程池工作
};
template <typename T>

threadPool<T>::threadPool()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t m_thread_numbers = std::thread::hardware_concurrency() * THREAD_NUMBER_FACTOR;
    if (m_thread_numbers == 0) {
        m_thread_numbers = 4; // 默认值，如果无法获取硬件并行度
    }
    m_stop = false;
    //线程池初始化 线程
    m_threads = new pthread_t[m_thread_numbers];
    if(!m_threads){
        throw std::exception();
    }
    for(int i=0;i<m_thread_numbers;i++){
        if(pthread_create(m_threads + i,nullptr,worker,this) != 0){
            delete[] m_threads;
            throw std::exception();
        }
        if(pthread_detach(m_threads[i])){
            delete[] m_threads;
            throw std::exception();
        }
    }
}
template <typename T>
threadPool<T>::~threadPool()
{   
    //停止所有线程
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_condition.notify_all();

    //清空工作队列
    m_work_queue.clear();
    delete[] m_threads;
}
template <typename T>
void* threadPool<T>::worker(void* arg){
    threadPool* pool = static_cast<threadPool*>(arg);
    pool->run();
    return nullptr;
}
template <typename T>
void threadPool<T>::run(){
    //从工作队列中取
    while(1){
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock,[this](){return !this->m_work_queue.empty()|| m_stop; });
        //停止工作
        if(m_stop && this->m_work_queue.empty()) return;
        auto task = m_work_queue.front();
        m_work_queue.pop_front();
        lock.unlock();  // 解锁，以便其他线程可以访问队列
        task->func(task->arg);        
    }
    
}
template <typename T>
void threadPool<T>::append(std::shared_ptr<T> arg){
    //向工作队列中添加
   {
        std::lock_guard<std::mutex> lock(m_mutex);
        //TODO:判断任务数量是否过多？
        m_work_queue.push_back(arg);
   }
    m_condition.notify_one();//通知一个线程处理
}


