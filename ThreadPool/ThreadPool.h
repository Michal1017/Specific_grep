#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <functional>
#include <filesystem>
#include <fstream>
#include <iostream>

class ThreadPool
{
public:
    // constructor take one argument which is number of threads we want to create
    ThreadPool(size_t num_threads);
    ~ThreadPool();

    // sumbit method is used to sumbit a new task to a tasks_ queue
    template <class F, class... Args>
    auto submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))>
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

private:
    // vector of created tasks
    std::vector<std::thread> threads_;
    // queue where tasks are stored
    std::queue<std::function<void()>> tasks_;
    // mutex which help to make sure if specific thread has exlusive access to tasks_
    std::mutex mutex_;
    // condition variable which allow the worker threads to wait until there are available tasks in task queue
    std::condition_variable condition_;
    // bool variable which inform us if working of thread pool is stoped
    bool stop_ = false;
};