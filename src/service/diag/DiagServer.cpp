//
// Created by 87837 on 2024/6/24.
//

#include "DiagServer.h"

int8_t DiagServer::configAddr(uint16_t NodeHandle, uint16_t PhyAddr, uint16_t FuncAddr, uint16_t RespAddr) {
    if (nodeMap.find(NodeHandle) == nodeMap.end()) {
        return 0;
    }
    Node *node = nodeMap[NodeHandle];
    node->diagConfig->PhyAddr = PhyAddr;
    node->diagConfig->FuncAddr = FuncAddr;
    node->diagConfig->RespAddr = RespAddr;
    return 1;
}

uint32_t DiagServer::sendByPhysical(uint16_t NodeHandle, uint8_t *data, uint32_t dataLength) {
    if (nodeMap.find(NodeHandle) == nodeMap.end()) {
        return 0;
    }
    Node *node = nodeMap[NodeHandle];
    auto *parsingDTO = new DiagSession();
    parsingDTO->data = data;
    parsingDTO->dataLength = dataLength;
    parsingDTO->addressingMode = physical;
    parsingDTO->id = DiagServer::generateDiagId(NodeHandle);
    auto *diagTransmitter = new DiagTransmitter(parsingDTO, node);
    auto *diagReceiver = new DiagReceiver(parsingDTO, node);
    diagMap.insert(std::pair<uint16_t, DiagSession *>(parsingDTO->id, parsingDTO));
    return parsingDTO->id;
}

uint32_t DiagServer::generateDiagId(uint16_t NodeHandle) {
    static uint32_t id;
    static uint16_t diagId;
    diagId++;
    id = NodeHandle << 16 | diagId;
    return id;
}

int8_t DiagServer::waitDiagComplete(uint32_t diagId) {
//    TODO: 等待诊断完成
    return 0;
}

