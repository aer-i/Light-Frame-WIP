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
        m_thread = std::thread(&Thread::threadLoop, this);
    }

    ~Thread()
    {
        if (m_thread.joinable())
        {
            this->wait();
            m_queueMutex.lock();
            m_executing = false;
            m_conditionVariable.notify_one();
            m_queueMutex.unlock();
            m_thread.join();
        }
    }

    auto enqueue(std::function<void()>&& task) noexcept -> void
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_jobQueue.push(std::move(task));
        m_conditionVariable.notify_one();
    }

    auto wait() noexcept -> void
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_conditionVariable.wait(lock, [this]{ return m_jobQueue.empty(); });
    }

private:
    auto threadLoop() noexcept -> void
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_conditionVariable.wait(lock, [this] { return !m_jobQueue.empty() || !m_executing; });
                if (!m_executing)
                {
                    break;
                }
                task = m_jobQueue.front();
            }
            task();
            {
                std::lock_guard<std::mutex> l(m_queueMutex);
                m_jobQueue.pop();
                m_conditionVariable.notify_one();
            }
        }
    }

private:
    std::thread m_thread;
    std::queue<std::function<void()>> m_jobQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_conditionVariable;
    bool m_executing = true;
};