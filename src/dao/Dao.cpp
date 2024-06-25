//
// Created by 87837 on 2024/6/2.
//

#pragma once

#include <io.h>
#include "../../include/SQLiteCpp/Database.h"
#include "../model/entity/Log.h"
#include "../model/entity/Diag.h"
#include "../exception/GlobalExceptionHandling.cpp"

class DBHelper {
private:
    SQLite::Database *db;
    char dbName[30] = "CaplUtil.db";

//    创建一个线程专门用来写入数据，读取则无需
    void CreateTable() {
        try {
            db->exec("CREATE TABLE IF NOT EXISTS log (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "level char(1), "
                     "tag TEXT, "
                     "message TEXT, "
                     "time INTEGER DEFAULT (strftime('%s', 'now')))");
//            创建表存储  DiagSession 以BLOB的形式存储
            db->exec("CREATE TABLE IF NOT EXISTS diagsession (id char(32) PRIMARY KEY, "
                     "session varchar(1024))");

        } catch (std::exception &e) {
            GlobalExceptionHandling(__FUNCTION__, e);
        }
    }

    explicit DBHelper() {
//        判断数据库是否存在，若存在则改名为备份，然后删除
        if (access(dbName, 0) == 0) {
            rename(dbName, "CaplUtil.db.bak");
            remove(dbName);
        }
        db = new SQLite::Database(dbName, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        CreateTable();
    }

public:
//    禁止拷贝构造
    DBHelper(const DBHelper &dbHelper) = delete;

    DBHelper &operator=(const DBHelper &dbHelper) = delete;

    ~DBHelper() {
        delete db;
        db = nullptr;
        remove(dbName);
    }

    static std::shared_ptr<DBHelper> &getInstance() {
        static std::shared_ptr<DBHelper> instance(new DBHelper());
        return instance;
    }

    void insertLog(const Log &log) {
//        捕获异常处理
        try {
            SQLite::Statement query(*db, "INSERT INTO log (level, tag, message) VALUES (?, ?, ?)");
            query.bind(1, log.level);
            query.bind(2, log.tag);
            query.bind(3, log.message);
            query.exec();
        } catch (std::exception &e) {
            GlobalExceptionHandling(__FUNCTION__, e);
        }
    }

    std::vector<Log> getLogs(int page, int pageSize) {
        std::vector<Log> logs;
        try {
            SQLite::Statement query(*db, "SELECT level, tag, message, time FROM log ORDER BY id DESC LIMIT ? OFFSET ?");
            query.bind(1, pageSize);
            query.bind(2, (page - 1) * pageSize);
            while (query.executeStep()) {
                Log log;
                log.level = static_cast<LogLevel>(query.getColumn(0).getInt());
                log.tag = query.getColumn(1).getText();
                log.message = query.getColumn(2).getText();
                log.time = query.getColumn(3).getInt64();
                logs.push_back(log);
            }
        } catch (std::exception &e) {
            GlobalExceptionHandling(__FUNCTION__, e);
        }
        return logs;
    }

    void count() {
        try {
            SQLite::Statement query(*db, "SELECT COUNT(*) FROM log");
            while (query.executeStep()) {
                cclPrintf("count = %d", query.getColumn(0).getInt());
            }
        } catch (std::exception &e) {
            GlobalExceptionHandling(__FUNCTION__, e);
        }
    }
//    插入DiagSession
//    void insertDiagSession(DiagSession diagSession) {
//        try {
//            SQLite::Statement query(*db, "INSERT INTO diagsession (id, session) VALUES (?, ?)");
//            query.bind(1, diagSession.diagSessionID);
////            转换为二进制数据
//            query.bind(2, diagSession.toBytes()->data(), diagSession.toBytes()->size());
//            query.exec();
//        } catch (std::exception &e) {
//            GlobalExceptionHandling(__FUNCTION__ , e);
//        }
//    }
//
//    DiagSession getDiagSession(uint32_t diagSessionID) {
//        DiagSession diagSession;
//        try {
//            SQLite::Statement query(*db, "SELECT session FROM diagsession WHERE id = ?");
//            query.bind(1, diagSessionID);
//            while (query.executeStep()) {
//                diagSession.fromBytes(query.getColumn(0).getString());
//            }
//        } catch (std::exception &e) {
//            GlobalExceptionHandling(__FUNCTION__ , e);
//        }
//        return diagSession;
//    }

};
