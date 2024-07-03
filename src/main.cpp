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
}


void OnMeasurementStart() {
    cclTimerSet(globalVar.timerID, cclTimeMilliseconds(300));

}

void OnMeasurementStop() {
    ThreadPool::getInstance()->~ThreadPool();
}

void OnCanMessage(struct cclCanMessage *message) {
//    打印收到报文的时间
//    cclPrintf("globalVar.runTime %lld", globalVar.runTime);
//    cclPrintf("OnCanMessage %lld", message->time);
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
        {"Debug", (CAPL_FARCALL) debug, "DeBug",                                        "print Debug",                                   'V', 0, "",     "",                 {""}},
        {"Debug_SendDiag", (CAPL_FARCALL) Debug_SendDiag, "DeBug", "Send a diagnostic message", 'V', 2, "BD", "\001\000", {"data", "dataLength"}},
//        Node相关
        {"Node_CreateNode", (CAPL_FARCALL) NodeService::createNode, "Node", "Create a node", 'L', 1, "L", "\000",                                                            {"nmId"}},
//        Diag
//        {"Diag_ConfigAddr",       (CAPL_FARCALL) DiagServer::configAddr,       "Diag",  "Config a Diag's address",                       'L', 4, "LLLL", "\000\000\000\000", {"NodeHandle", "PhyAddr", "FuncAddr", "RespAddr"}},
//        {"Diag_SendByPhysical",   (CAPL_FARCALL) DiagServer::sendByPhysical,   "Diag",  "Send a diagnostic message by physical address", 'L', 3, "LBL",  "\000\001\000",     {"NodeHandle", "data",    "dataLength"}},
//        {"Diag_WaitDiagComplete", (CAPL_FARCALL) DiagServer::waitDiagComplete, "Diag",  "Wait for the diagnostic to complete",           'L', 1, "L",    "\000",             {"diagId"}},
        {0,                0}
};
CAPLEXPORT CAPL_DLL_INFO4 *caplDllTable4 = table;