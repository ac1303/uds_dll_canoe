//
// Created by fanshuhua on 2024/7/3.
//

#include "DiagService.h"

int DiagService::configAddr(uint16_t nodeHandle, uint16_t phyAddr, uint16_t funcAddr, uint16_t respAddr) {
    if (nodeMap.find(nodeHandle) == nodeMap.end()) {
        return -1;
    }
    Node *node = nodeMap[nodeHandle];
    node->diagConfig->PhyAddr = phyAddr;
    node->diagConfig->FuncAddr = funcAddr;
    node->diagConfig->RespAddr = respAddr;
//    生成对应的广播器和监听器
    auto *diagEventMulticaster = new DiagEventMulticaster();
    auto *diagSend = new DiagSend(node, diagEventMulticaster);
    auto *diagReceive = new DiagReceive(node, diagEventMulticaster);
    return 1;
}
