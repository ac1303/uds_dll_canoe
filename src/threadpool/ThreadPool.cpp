//
// Created by 87837 on 2024/6/2.
//
#pragma once

#include <vector>
#include <thread>
#include <functional>
#include <mutex>
#include <queue>
#include <future>
#include <iostream>

class ThreadPool {

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;

    explicit ThreadPool(int numThreads) : stop(false) {
        for (int i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                for (;;) {
                    std::function < void() > task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] {
                            return this->stop || !this->tasks.empty();
                        });
                        if (this->stop && this->tasks.empty()) {
                            return;
                        }
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

public:
    static std::shared_ptr<ThreadPool> &getInstance() {
//        static ThreadPool instance(2);
        static std::shared_ptr<ThreadPool> instance(new ThreadPool(2));
        return instance;
    }

    template<class F, class... Args>
    void enqueue(F &&f, Args &&... args) {
        auto task = [Func = std::forward<F>(f)] { return Func(); };
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::move(task));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker: workers) {
            worker.join();
        }
    }

//    禁止拷贝构造
    ThreadPool(const ThreadPool &threadPool) = delete;

    ThreadPool &operator=(const ThreadPool &threadPool) = delete;

    bool stop;
};
