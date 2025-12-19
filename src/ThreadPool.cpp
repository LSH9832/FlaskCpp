#include "FlaskCpp/ThreadPool.h"
#include <stdexcept>

// Конструктор
ThreadPool::ThreadPool(size_t minThreads, size_t maxThreads, bool verbose)
    : stop(false), minThreads(minThreads), maxThreads(maxThreads), currentThreads(0), verbose(verbose), threadsToTerminate(0)
{
    if (minThreads > maxThreads) {
        throw std::invalid_argument("minThreads cannot be greater than maxThreads");
    }

    // Запускаем минимальное количество потоков
    for (size_t i = 0; i < minThreads; ++i) {
        addThread();
    }

    // Запускаем поток мониторинга нагрузки
    monitorThread = std::thread(&ThreadPool::monitorLoad, this);
}

// Деструктор
ThreadPool::~ThreadPool()
{
    shutdown();
}

// Добавление нового потока
void ThreadPool::addThread()
{
    workers.emplace_back([this]() {
        while (true) {
            PrioritizedTask pt;
            {
                std::unique_lock<std::mutex> lock(this->queueMutex);
                this->condition.wait(lock, [this]() { return this->stop.load() || !this->tasks.empty() || this->threadsToTerminate.load() > 0; });
                
                if (this->stop.load() && this->tasks.empty())
                    return;
                
                if (this->threadsToTerminate.load() > 0 && this->tasks.empty()) {
                    this->threadsToTerminate.fetch_sub(1);
                    currentThreads.fetch_sub(1);
                    if (verbose) {
                        std::cout << "ThreadPool: current size: " << currentThreads.load() << std::endl;
                    }
                    return;
                }

                if (!this->tasks.empty()) {
                    pt = std::move(this->tasks.top());
                    this->tasks.pop();
                } else {
                    continue;
                }
            }
            pt.task();
        }
    });
    currentThreads.fetch_add(1);
    if (verbose) {
        std::cout << "ThreadPool: Добавлен поток. Текущий размер пула: " << currentThreads.load() << std::endl;
    }
}

// Метод мониторинга нагрузки
void ThreadPool::monitorLoad()
{
    while (!stop.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        size_t taskCount;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            taskCount = tasks.size();
        }

        // Логика увеличения пула потоков
        if (taskCount > currentThreads.load() && currentThreads.load() < maxThreads) {
            size_t threadsToAdd = std::min(taskCount - currentThreads.load(), maxThreads - currentThreads.load());
            for (size_t i = 0; i < threadsToAdd; ++i) {
                addThread();
                if (verbose) {
                    std::cout << "ThreadPool: Добавлен поток. Текущий размер пула: " << currentThreads.load() << std::endl;
                }
            }
        }

        // Логика уменьшения пула потоков
        else if (taskCount < currentThreads.load() && currentThreads.load() > minThreads) {
            size_t excessThreads = currentThreads.load() - minThreads;
            // Решаем уменьшить пул на 1 поток за раз для плавности
            size_t threadsToRemove = 1;
            if (excessThreads < threadsToRemove) {
                threadsToRemove = excessThreads;
            }
            threadsToTerminate.fetch_add(threadsToRemove);
            condition.notify_all(); // Уведомляем все потоки о возможном завершении
            if (verbose) {
                std::cout << "ThreadPool: Запрошено завершение " << threadsToRemove << " потока(ов). Текущий размер пула: " << currentThreads.load() << std::endl;
            }
        }
    }
}

// Метод остановки пула потоков
void ThreadPool::shutdown()
{
    try
    {
        bool expected = false;
        if(stop.compare_exchange_strong(expected, true)) {
            condition.notify_all();
            // std::cout << "num workers: " << workers.size() << std::endl;
            for(std::thread &worker: workers)
                if(worker.joinable())
                    worker.join();
            // std::cout << "stop monitor thread" << std::endl;
            if(monitorThread.joinable())
                monitorThread.join();
            if (verbose) {
                std::cout << "ThreadPool: all stopped." << std::endl;
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    
}

size_t ThreadPool::getMinThreads()
{
    return minThreads;
}

size_t ThreadPool::getMaxThreads()
{
    return maxThreads;
}