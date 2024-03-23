#pragma once
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>

class Thread
{
public:
    Thread()
    {
        m.thread = std::thread(&Thread::threadLoop, this);
    }

    ~Thread()
    {
        if (m.thread.joinable())
        {
            this->wait();
            m.queueMutex.lock();
            m.executing = false;
            m.conditionVariable.notify_one();
            m.queueMutex.unlock();
            m.thread.join();
        }
    }

    auto enqueue(std::function<void()>&& task) noexcept -> void
    {
        std::lock_guard<std::mutex> lock(m.queueMutex);
        m.jobQueue.push(std::move(task));
        m.conditionVariable.notify_one();
    }

    auto wait() noexcept -> void
    {
        std::unique_lock<std::mutex> lock(m.queueMutex);
        m.conditionVariable.wait(lock, [this]{ return m.jobQueue.empty(); });
    }

private:
    auto threadLoop() noexcept -> void
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m.queueMutex);
                m.conditionVariable.wait(lock, [this] { return !m.jobQueue.empty() || !m.executing; });
                if (!m.executing)
                {
                    break;
                }
                task = m.jobQueue.front();
            }
            task();
            {
                std::lock_guard<std::mutex> l(m.queueMutex);
                m.jobQueue.pop();
                m.conditionVariable.notify_one();
            }
        }
    }

private:
    struct M
    {
        std::thread thread;
        std::queue<std::function<void()>> jobQueue;
        std::mutex queueMutex;
        std::condition_variable conditionVariable;
        bool executing = true;
    } m;
};