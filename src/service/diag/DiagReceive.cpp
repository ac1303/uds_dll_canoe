//
// Created by fanshuhua on 2024/7/3.
//

#include "DiagReceive.h"

bool DiagReceive::isDiagMessage(cclCanMessage *canMessage) {
    return canMessage->id == node->diagConfig->PhyAddr
           || canMessage->id == node->diagConfig->FuncAddr
           || canMessage->id == node->diagConfig->RespAddr;
}

bool DiagReceive::onEvent(EventType type, void *event) {
    switch (type) {
        case CanEvent:
            return isDiagMessage((cclCanMessage *) event);
        default:
            return false;
    }
}

void DiagReceive::callback(void *event) {
//    根据ID执行不同的操作
    auto *canMessage = (cclCanMessage *) event;
    if (canMessage->id == node->diagConfig->PhyAddr) {
        handlePhyAddr(canMessage);
    } else if (canMessage->id == node->diagConfig->FuncAddr) {
        handleFuncAddr(canMessage);
    } else if (canMessage->id == node->diagConfig->RespAddr) {
        handleRespAddr(canMessage);
    }
}

void DiagReceive::handlePhyAddr(cclCanMessage *canMessage) {
//
}

void DiagReceive::handleRespAddr(cclCanMessage *canMessage) {
//    判断响应报文类型，单帧、多帧、流控帧、否定响应等
// 1、当第一个字节的高四位为3时，代表是流控帧
    if ((canMessage->data[0] & 0xF0) == 0x30) {
//        通知发送模块，流控帧已经接收
        diagEventMulticaster->notify(DiagReceiveFlowControlEvent, canMessage);
    }
}

void DiagReceive::handleFuncAddr(cclCanMessage *canMessage) {

}
