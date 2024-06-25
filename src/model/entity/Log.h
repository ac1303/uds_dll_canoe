//
// Created by fanshuhua on 2024/6/1.
//

#ifndef CAPLUTILS_LOG_H
#define CAPLUTILS_LOG_H

#include <string>
#include <ctime>
#include "SQLiteCpp/Column.h"

enum LogLevel {
    LOG_INFO = 0,
    LOG_DEBUG = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3,
    LOG_FATAL = 4
};

typedef struct LogStruct {
    LogLevel level;
    std::string tag;
    std::string message;
    std::time_t time;
} Log;
#endif //CAPLUTILS_LOG_H
