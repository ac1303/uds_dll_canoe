//
// Created by fanshuhua on 2024/5/31.
//
// AOP.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "AOP.h"
#include <functional>
#include <chrono>

using namespace std;
using namespace std::chrono;

class Timer {
public:
    Timer() : m_begin(high_resolution_clock::now()) {}

    void reset() { m_begin = high_resolution_clock::now(); }

    // 默认输出毫秒
    template<typename Duration = microseconds>
    int64_t elapsed() const {
        return duration_cast<Duration>(high_resolution_clock::now() - m_begin).count();
    }

    // 微秒
    int64_t elapsed_micro() const {
        return elapsed<microseconds>();
    }

    // 纳秒
    int64_t elapsed_nano() const {
        return elapsed<nanoseconds>();
    }

    // 秒
    int64_t elapsed_seconds() const {
        return elapsed<seconds>();
    }

    int64_t elapsed_minutes() const {
        return elapsed<minutes>();
    }

    int64_t elasped_hours() const {
        return elapsed<hours>();
    }

private:
    time_point<high_resolution_clock> m_begin;
};

struct AA {
    void Before(int i) {
        cout << "Before from AA " << i << endl;
    }

    void After(int i) {
        cout << "After from AA " << i << endl;
    }
};

struct BB {
    void Before(int i) {
        cout << "Before from BB " << i << endl;
    }

    void After(int i) {
        cout << "After from BB " << i << endl;
    }
};

struct CC {
    void Before() {
        cout << "Before from CC " << endl;
    }

    void After() {
        cout << "After from CC " << endl;
    }
};

struct DD {
    void Before() {
        cout << "Before from DD " << endl;
    }

    void After() {
        cout << "After from DD " << endl;
    }
};

void GT() {
    cout << "real GT function" << endl;
}

void HT(int a) {
    cout << "real HT function: " << a << endl;
}

struct TimeElapsedAspect {
    void Before(int i) {

    }

    void After(int i) {
        cout << "time elapsed: " << m_t.elapsed() - m_lastTime << endl;
    }

private:
    double m_lastTime;
    Timer m_t;
};

struct LoggingAspect {
    void Before(int i) {
        cout << "entering" << endl;
    }

    void After(int i) {
        cout << "leaving" << endl;
    }
};

void foo(int a) {
    cout << "real HT function: " << a << endl;
}

int main() {
    //织入普通函数
    function<void(int)> f = bind(&HT, placeholders::_1);
    Invoke<AA, BB>(function < void(int) > (bind(&HT, placeholders::_1)), 1);
    // 组合了两个切面AA,BB
    Invoke<AA, BB>(f, 1);

    // 织入普通函数
    Invoke<CC, DD>(&GT);
    Invoke<AA, BB>(&HT, 1);
    // 织入lambda表达式
    Invoke<AA, BB>([](int i) {}, 1);
    Invoke<CC, DD>([] {});
    {
        cout << "--------------------" << endl;
        Invoke<LoggingAspect, TimeElapsedAspect>(&foo, 1);
        cout << "----------------------" << endl;
        Invoke<TimeElapsedAspect, LoggingAspect>(&foo, 1);
        cout << "--------------------" << endl;
    }
}
