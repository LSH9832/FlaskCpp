#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>
#include <chrono>
#include <iostream> // Для std::cout и std::endl

// Структура для задач с приоритетом
struct PrioritizedTask {
    int priority; // Чем меньше число, тем выше приоритет
    std::function<void()> task;
    
    bool operator<(const PrioritizedTask& other) const {
        // Для priority_queue, которая по умолчанию максимальная, инвертируем сравнение
        return priority > other.priority;
    }
};

class ThreadPool {
public:
    ThreadPool(size_t minThreads, size_t maxThreads, bool verbose = false);
    ~ThreadPool();

    // Запускает задачу с указанным приоритетом и возвращает future для получения результата
    template<class F, class... Args>
    auto enqueue(int priority, F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared< std::packaged_task<return_type()> >(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
                
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);

            if(stop.load())
                throw std::runtime_error("enqueue on stopped ThreadPool");

            tasks.emplace(PrioritizedTask{priority, [task](){ (*task)(); }});
        }
        condition.notify_one();
        return res;
    }

    // Останавливает пул потоков
    void shutdown();


    size_t getMinThreads();

    size_t getMaxThreads();

private:
    // Рабочие потоки
    std::vector<std::thread> workers;

    // Приоритетная очередь задач
    std::priority_queue<PrioritizedTask> tasks;

    // Мьютекс для защиты очереди задач
    std::mutex queueMutex;

    // Условная переменная для уведомления потоков
    std::condition_variable condition;

    // Флаги остановки пула
    std::atomic<bool> stop;

    // Управление размером пула
    size_t minThreads;
    size_t maxThreads;
    std::atomic<size_t> currentThreads;

    // Флаг verbose
    bool verbose;

    // Мониторинг нагрузки для динамического изменения размера пула
    std::thread monitorThread;
    void monitorLoad();

    // Добавление нового потока
    void addThread();

    // Логика уменьшения пула потоков
    std::atomic<int> threadsToTerminate; // Количество потоков, которые должны завершиться
};

#endif // THREADPOOL_H
