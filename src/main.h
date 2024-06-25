//
// Created by 87837 on 2024/6/2.
//

#ifndef DLLTEST_MAIN_H
#define DLLTEST_MAIN_H

#include "vector/CCL/CCL.h"
#include "vector/CCL/CCL.cpp"
#include "vector/CCL/cdll.h"
#include "vector/CCL/VIA_CAN.h"

typedef struct GlobalVar {
    long long runTime; // 微秒
    int timerID;
    uint16 VIAChannel;
    VIACan *canBus;
} GlobalVar;
static GlobalVar globalVar;

//=============
#include "threadpool/ThreadPool.cpp"
#include "dao/Dao.cpp"
#include "utils/GlobalUtils.cpp"
#include "model/entity/Log.h"
#include "model/entity/Node.h"
#include "model/entity/Diag.h"

#include "service/event/EventListener.h"
#include "service/event/EventMulticaster.h"
#include "service/event/EventMulticaster.cpp"

#include "service/node/NodeService.h"
#include "service/node/NodeService.cpp"
#include "service/diag/DiagServer.h"
#include "service/diag/DiagServer.cpp"

extern void OnMeasurementPreStart();

extern void OnMeasurementStart();

extern void OnMeasurementStop();

extern void OnTimer(long long time, int timerID);

extern void OnCanMessage(struct cclCanMessage *message);

#endif //DLLTEST_MAIN_H
