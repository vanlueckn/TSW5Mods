#include "mainthreadqueue.hpp"
#include <mutex>

MainThreadQueue& MainThreadQueue::get_instance()
{
    static MainThreadQueue instance;
    return instance;
}

void MainThreadQueue::add_function(const std::function<void()>& func)
{
    std::lock_guard lock(queue_mutex_);
    main_thread_queue_.push(func);
}

void MainThreadQueue::execute_next()
{
    std::lock_guard lock(queue_mutex_);
    if (!main_thread_queue_.empty())
    {
        const std::function<void()> func = main_thread_queue_.front();
        main_thread_queue_.pop();
        func();
    }
}

bool MainThreadQueue::is_empty() const
{
    std::lock_guard lock(queue_mutex_);
    return main_thread_queue_.empty();
}

MainThreadQueue::MainThreadQueue()
= default;
