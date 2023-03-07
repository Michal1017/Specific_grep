#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t num_threads)
{
    for (size_t i = 0; i < num_threads; ++i)
    {
        // add new thread to thread_ vector where lambda function, which implement behavior of new thread,
        // is passed as argument to emplace_back function
        threads_.emplace_back([this]
                              {
                while (true) {
                    std::unique_lock<std::mutex> lock(mutex_);
                    // wait until there are available tasks 
                    condition_.wait(lock, [this] { return !tasks_.empty() || stop_; });
                    // check if thread pool is shut down and there are no more tasks to run
                    if (stop_ && tasks_.empty()) {
                        return;
                    }
                    // move a new task to do to a new variable
                    auto task = std::move(tasks_.front());
                    // remove task from the front of queue
                    tasks_.pop();
                    // realise the lock, so other threads can have access to a tasks_ queue
                    lock.unlock();
                    // execute a task
                    task();
                } });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // set a stop_ flag to true so inform working threads to exit infinite loop which is in contructor
        stop_ = true;
    }
    // wake up all threads so they will check stop_ flag
    condition_.notify_all();
    // finish executing of all threads
    for (auto &thread : threads_)
    {
        thread.join();
    }
}

// because of we don't know what task we receive, we declarate tamplate declaration
template <class F, class... Args>
auto ThreadPool::submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))>
{
    // deduce a return type of f function, which will be our task
    using return_type = decltype(f(args...));
    // create shared pointer to a object which store a f function and arguments
    auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    // future object is created to hold the raturn value of f function
    std::future<return_type> result = task->get_future();
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // if thread pool is stopped throw exception
        if (stop_)
        {
            throw std::runtime_error("submit on stopped ThreadPool");
        }
        // add new task to a tasks_ queue
        tasks_.emplace([task]()
                       { (*task)(); });
    }
    // notify a one thread because new task has been added
    condition_.notify_one();
    // return a future object which store return value of a f function
    return result;
}