//
// Created by 87837 on 2024/6/2.
//



#include "main.h"

void cclOnDllLoad() {
    cclSetMeasurementPreStartHandler(&OnMeasurementPreStart);
    cclSetMeasurementStartHandler(&OnMeasurementStart);
    cclSetMeasurementStopHandler(&OnMeasurementStop);
}

void OnMeasurementPreStart() {
    cclPrintf("OnMeasurementPreStart");
//    开启定时器
    globalVar.timerID = cclTimerCreate(&OnTimer);
    globalVar.VIAChannel = gMasterLayer->mChannel;
    globalVar.canBus = gCanBusContext[globalVar.VIAChannel].mBus;
    cclCanSetMessageHandler(globalVar.VIAChannel, CCL_CAN_ALLMESSAGES, &OnCanMessage);
//    for (int i = 0; i < 10; ++i) {
//        ThreadPool::getInstance().enqueue([i] {
//            cclPrintf("Task %d is running \n ", i);
//            std::this_thread::sleep_for(std::chrono::seconds(1));
//            cclPrintf("Task %d is done \n ", i);
//        });
//    }

//  初始化DBHelper,插入当前时间
// 使用线程往日志里面插入一千条日志
//    for (int i = 0; i < 1000; ++i) {
//        ThreadPool::getInstance()->enqueue([i] {
//            Log log;
//            log.level = LOG_INFO;
//            log.tag = "tag";
//            log.message = "message " + std::to_string(i);
//            DBHelper::getInstance()->insertLog(log);
//        });
//    }
//    std::vector<Log> logs = DBHelper::getInstance()->getLogs(1, 10);
//    for (auto &log : logs) {
//        cclPrintf("level: %d, tag: %s, message: %s , time: %s", log.level, log.tag.c_str(), log.message.c_str(), cclTimeToString(log.time));
//    }

//插入 insertDiagSession
//    DiagSession diagSession = {
//            .diagSessionID =  123456,
//            .diagSessionType =  sent,
//            .sendData =  std::vector<cclCanMessage *> (),
//            .receiveData =  std::vector<cclCanMessage *>() ,
//    };
//    cclCanMessage message = {
//            .id = 0x123,
//            .flags = 0,
//            .dir = kVIA_Tx,
//            .dataLength = 8,
//            .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,0x09}
//
//    };
//    cclCanMessage message2 = {
//            .id = 0x456,
//            .flags = 0,
//            .dir = kVIA_Rx,
//            .dataLength = 8,
//            .data = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18}
//
//    };
//    diagSession.sendData.push_back(&message);
//    diagSession.sendData.push_back(&message2);
//    DBHelper::getInstance()->insertDiagSession(diagSession);
//    DiagSession diagSession1 = DBHelper::getInstance()->getDiagSession(123456);
//    cclPrintf("diagSession1.diagSessionID = %d", diagSession1.diagSessionID);
//    cclPrintf("diagSession1.diagSessionType = %d", diagSession1.diagSessionType);
//    cclPrintf("diagSession1.sendData.size() = %d", diagSession1.sendData.size());
//    cclPrintf("diagSession1.receiveData.size() = %d", diagSession1.receiveData.size());
////    打印sendData里面所有的数据
//    for (int i = 0; i < diagSession1.sendData.size(); ++i) {
//        cclPrintf("diagSession1.sendData[%d]->id = %x", i, diagSession1.sendData[i]->id);
//        cclPrintf("diagSession1.sendData[%d]->flags = %x", i, diagSession1.sendData[i]->flags);
//        cclPrintf("diagSession1.sendData[%d]->dataLength = %x", i, diagSession1.sendData[i]->dataLength);
//        cclPrintf("diagSession1.sendData[%d]->dir = %x", i, diagSession1.sendData[i]->dir);
//        for (int j = 0; j < diagSession1.sendData[i]->dataLength; ++j) {
//            cclPrintf("diagSession1.sendData[%d]->data[%d] = %x", i, j, diagSession1.sendData[i]->data[j]);
//        }
//    }
//    cclWrite("=============================================RX=============================================");
//    for (int i = 0; i < diagSession1.receiveData.size(); ++i) {
//        cclPrintf("diagSession1.receiveData[%d]->id = %x", i, diagSession1.receiveData[i]->id);
//        cclPrintf("diagSession1.receiveData[%d]->flags = %x", i, diagSession1.receiveData[i]->flags);
//        cclPrintf("diagSession1.receiveData[%d]->dataLength = %x", i, diagSession1.receiveData[i]->dataLength);
//        cclPrintf("diagSession1.receiveData[%d]->dir = %x", i, diagSession1.receiveData[i]->dir);
//        for (int j = 0; j < diagSession1.receiveData[i]->dataLength; ++j) {
//            cclPrintf("diagSession1.receiveData[%d]->data[%d] = %x", i, j, diagSession1.receiveData[i]->data[j]);
//        }
//    }
}


void OnMeasurementStart() {
    cclTimerSet(globalVar.timerID, cclTimeMilliseconds(300));

}

void OnMeasurementStop() {
//    cclPrintf("OnMeasurementStop");
////    打印当前时间
//    auto now = std::chrono::system_clock::now();
//    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
//    cclPrintf("Now is %s", cclTimeToString(now_time));
//    将线程池中的任务全部执行后销毁线程池
    ThreadPool::getInstance()->~ThreadPool();
//    查询当前数据库里有多少数据
//    DBHelper::getInstance()->count();
//    std::vector<Log> logs = DBHelper::getInstance()->getLogs(1, 10);
//    for (auto &log : logs) {
//        cclPrintf("level: %d, tag: %s, message: %s , time: %s", log.level, log.tag.c_str(), log.message.c_str(), cclTimeToString(log.time));
//    }
}

void OnCanMessage(struct cclCanMessage *message) {
//    打印收到报文的时间
//    cclPrintf("globalVar.runTime %lld", globalVar.runTime);
    cclPrintf("OnCanMessage %lld", message->time);
    EventMulticaster::getInstance()->notify(CanEvent, message);
}

void OnTimer(long long time, int timerID) {
//    cclPrintf("OnTimer %lld", time);
    globalVar.runTime = time;
    EventMulticaster::getInstance()->notify(TimeEvent, &time);
    cclTimerSet(globalVar.timerID, cclTimeMicroseconds(100));
}


//============================================================================
CAPL_DLL_INFO4 table[] = {
        {CDLL_VERSION_NAME,
                           (CAPL_FARCALL) CDLL_VERSION,
                                                          "",
                                                                   "",
                                                                                                CAPL_DLL_CDECL,
                                                                                                     0xabcd,
                CDLL_EXPORT},
        {"Debug",          (CAPL_FARCALL) debug,          "Debug", "Debug",                     'V', 0, "",   "",         {""}},
        {"Debug_SendDiag", (CAPL_FARCALL) Debug_SendDiag, "DeBug", "Send a diagnostic message", 'V', 2, "BD", "\001\000", {"data", "dataLength"}},
        {0,                0}
};
CAPLEXPORT CAPL_DLL_INFO4 *caplDllTable4 = table;