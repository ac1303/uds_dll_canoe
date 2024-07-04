//
// Created by fanshuhua on 2024/7/3.
//

#ifndef DLLTEST_DIAGSERVICE_H
#define DLLTEST_DIAGSERVICE_H

#include "DiagParsing.cpp"
#include "DiagSend.cpp"
#include "DiagReceive.cpp"

enum DiagServiceResult {
    Success = 0,
    Fail = -1,
    NoFound = -2,
};
class DiagService {
public:
    static DiagServiceResult configAddr(uint16_t nodeHandle, uint16_t phyAddr, uint16_t funcAddr, uint16_t respAddr);

    static uint32_t send(uint16_t nodeHandle, uint8_t *data, uint16_t len, AddressingMode type = Physical);
};


#endif //DLLTEST_DIAGSERVICE_H
