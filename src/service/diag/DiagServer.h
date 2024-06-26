//
// Created by 87837 on 2024/6/24.
//

#ifndef DLLTEST_DIAGSERVER_H
#define DLLTEST_DIAGSERVER_H

#include "DiagTransmitter.h"
#include "DiagTransmitter.cpp"
#include "DiagReceiver.h"
#include "DiagReceiver.cpp"

static std::map<uint16_t, DiagSession *> diagMap = std::map<uint16_t, DiagSession *>();

class DiagServer {
private:
//    生成唯一的诊断ID，根据节点ID和诊断ID生成
    static uint32_t generateDiagId(uint16_t NodeHandle);

public:
//    配置功能寻址，物理存在，响应地址
    static int8_t configAddr(uint16_t NodeHandle, uint16_t PhyAddr, uint16_t FuncAddr, uint16_t RespAddr);

//    等待诊断完成
    static int waitDiagComplete(uint32_t diagId);

    static uint32_t sendByPhysical(uint16_t NodeHandle, uint8_t *data, uint32_t dataLength);
};


#endif //DLLTEST_DIAGSERVER_H
