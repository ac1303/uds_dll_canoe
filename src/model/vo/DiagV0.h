//
// Created by fanshuhua on 2024/6/20.
//

#ifndef DLLTEST_DIAGSENDDATAV0_H
#define DLLTEST_DIAGSENDDATAV0_H

#include "../entity/Diag.h"

// 诊断状态
enum DiagSessionState {
    none = 0,
//    发送完成
    sendComplete,
//    接收完成
    received,
//    发送失败
    sendFailed,
//    接收失败
    receiveFailed,
//    流控帧溢出
    flowControlOverflow,
//    异常流控帧
    flowControlError,
//    超时
    AsTimeout,
    ArTimeout,
    BsTimeout,
    BrTimeout,
    CsTimeout,
    CrTimeout,
    P2ClientTimeout,
    P2ClientExTimeout,
};
//寻址方式，物理地址或功能地址
enum AddressingMode {
    physical = 0,
    functional = 1,
};
typedef struct DiagSession {
    uint32_t id;
    AddressingMode addressingMode = physical;
    DiagSessionState diagSessionState;  // 状态
    std::vector<cclCanMessage *> sendData;   // 已发送的数据
    std::vector<cclCanMessage *> receiveData; // 已接收的数据
    uint32_t dataLength = 0;
    uint8_t *data = nullptr;
    bool parsed = false;// 解析是否完成？
    uint32_t offset = 0;// 偏移量
    uint8_t SN = 0;// 连续帧序号
} DiagSession;
#endif //DLLTEST_DIAGSENDDATAV0_H
