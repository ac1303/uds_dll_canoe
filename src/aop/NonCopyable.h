//
// Created by fanshuhua on 2024/5/31.
//

#ifndef CAPLUTILS_NONCOPYABLE_H
#define CAPLUTILS_NONCOPYABLE_H
#pragma once

class NonCopyable {
protected:
    NonCopyable() = default;

    ~NonCopyable() = default;

    // 禁用复制构造
    NonCopyable(const NonCopyable &) = delete;

    // 禁用赋值构造
    NonCopyable &operator=(const NonCopyable &) = delete;
};

#endif //CAPLUTILS_NONCOPYABLE_H
