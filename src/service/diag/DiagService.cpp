//
// Created by fanshuhua on 2024/7/3.
//

#include "DiagService.h"

DiagServiceResult DiagService::configAddr(uint16_t nodeHandle, uint16_t phyAddr, uint16_t funcAddr, uint16_t respAddr) {
    if (nodeMap.find(nodeHandle) == nodeMap.end()) {
        return NoFound;
    }
    Node *node = nodeMap[nodeHandle];
    node->diagConfig->PhyAddr = phyAddr;
    node->diagConfig->FuncAddr = funcAddr;
    node->diagConfig->RespAddr = respAddr;
    auto *diagSend = new DiagSend(node);
    auto *diagReceive = new DiagReceive(node);
    return Success;
}

uint32_t DiagService::send(uint16_t nodeHandle, uint8_t *data, uint16_t len, AddressingMode type) {
    if (nodeMap.find(nodeHandle) == nodeMap.end()) {
        return NoFound;
    }
    Node *node = nodeMap[nodeHandle];
    auto *diagSession = new DiagSession();
    // 1. 创建session
//    TODO ID暂定为0
    diagSession->id = 10000001;
    diagSession->addressingMode = type;
    diagSession->diagSessionState = DiagSessionState::sendUnfinished;
    diagSession->errorStatus = 0;
    diagSession->dataLength = len;
    diagSession->data = data;
    diagSession->parsed = false;
    diagSession->offset = 0;
    diagSession->SN = 0;
    // 2. 添加到发送队列
    node->diagConfig->diagEventMulticaster->notify(EventType::DiagAddSessionEvent, diagSession);
    return diagSession->id;
}
