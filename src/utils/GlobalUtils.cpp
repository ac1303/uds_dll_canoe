//
// Created by 87837 on 2024/6/5.
//

//#include "../service/diag/DiagTransmitter.h"
#include "../model/entity/Node.h"

static char *cclTimeToString(std::time_t time) {
    char *buffer = (char *) malloc(80);
    struct tm *tm;
    tm = localtime(&time);
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", tm);
    return buffer;
}

static void debug() {
    cclPrintf("debug");
}

static void Debug_SendDiag(uint8_t *data, uint32_t dataLength) {
    Node *node = new Node();
    auto *parsingDTO = new DiagSession();
    parsingDTO->data = data;
    parsingDTO->dataLength = dataLength;

    cclPrintf("dataLength: %d", parsingDTO->dataLength);
//    auto *diagTransmitter = new DiagTransmitter(parsingDTO, node);
}