#ifndef  __THREADPOOL_H
#define  __THREADPOOL_H
#include <vector>
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <future>
#include  <stdio.h>
class ThreadPool
{
private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;

    std::mutex m_queue_mutex;
    std::condition_variable m_condition;
    bool stop = false;
    size_t MaxThreadCount = 10;
public:
    ThreadPool(size_t count)
    {
        size_t pools = m_workers.size();
        for(;pools<MaxThreadCount && count>0;--count){
            m_workers.emplace_back([this]()->void{
                while(true){
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->m_queue_mutex);
                        this->m_condition.wait(lock, [this]()->bool{
                            return this->stop || !this->m_tasks.empty();
                        });
                        if(this->stop && this->m_tasks.empty()){
                            return;
                        }
                        task = std::move(this->m_tasks.front());
                        this->m_tasks.pop();
                    }
                    task();
                }
            });
        }
    }
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f),std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            if(stop){
                throw std::runtime_error("qnueue on stopped ThreadPool");
            }
            m_tasks.emplace([task](){
                (*task)();
            });
        }
        m_condition.notify_one();
        return res;
    }
    inline ~ThreadPool()
    {
        //printf("ThreadPool clean in....");
        m_queue_mutex.lock();
        stop = true;
        m_queue_mutex.unlock();
        m_condition.notify_all();
        for(std::thread& worker: m_workers){
            worker.join();
        }
        //printf("ThreadPool clean ok....");
    }
};

#endif