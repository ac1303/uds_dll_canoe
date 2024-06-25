//
// Created by fanshuhua on 2024/6/19.
//

#ifndef DLLTEST_NODE_H
#define DLLTEST_NODE_H

//typedef struct NodeConfig{
//    uint16_t nodeID;
//    uint16_t BaseId;
//    uint8_t EcuId;
//    uint16_t PhyAddr;
//    uint16_t FuncAddr;
//    uint16_t RespAddr;
//    NetworkLayerTime *networkLayerTime;
//    SessionLayerTime *sessionLayerTime;
//}NodeConfig;
typedef struct Node {
    uint16_t NodeHandle = 0;
    uint16_t BaseId = 0;
    uint8_t EcuId = 0;
    DiagConfig *diagConfig = new DiagConfig();
} Node;
#endif //DLLTEST_NODE_H
