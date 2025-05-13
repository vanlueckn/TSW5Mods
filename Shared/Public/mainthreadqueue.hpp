#pragma once

#include <queue>
#include <functional>
#include <mutex>

class MainThreadQueue
{
public:
    // Get the singleton instance
    static MainThreadQueue& get_instance();
    MainThreadQueue(const MainThreadQueue&) = delete;
    MainThreadQueue& operator=(const MainThreadQueue&) = delete;

    void add_function(const std::function<void()>& func);
    void execute_next();
    bool is_empty() const;

private:
    MainThreadQueue();
    std::queue<std::function<void()>> main_thread_queue_;
    mutable std::mutex queue_mutex_;
};
