//
// Created by fanshuhua on 2024/7/3.
//

#ifndef DLLTEST_DIAGSERVICE_H
#define DLLTEST_DIAGSERVICE_H

#include "DiagSend.cpp"
#include "DiagReceive.cpp"

class DiagService {
public:
//    "NodeHandle", "PhyAddr", "FuncAddr", "RespAddr"
    static int configAddr(uint16_t nodeHandle, uint16_t phyAddr, uint16_t funcAddr, uint16_t respAddr);
};


#endif //DLLTEST_DIAGSERVICE_H
