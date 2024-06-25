//
// Created by fanshuhua on 2024/6/4.
//

// 只编译一次
#pragma once

// 处理全局异常
#include <exception>

void GlobalExceptionHandling(const char *functionName, std::exception &e) {
    cclPrintf("Exception in %s: %s", functionName, e.what());
//    关闭线程池
    ThreadPool::getInstance()->~ThreadPool();
//    将数据库的名字改为备份
    rename("CaplUtil.db", "CaplUtil.db.bak");
    gVIAService->Stop();
}
