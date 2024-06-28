//
// Created by fanshuhua on 2024/6/20.
//

#ifndef DLLTEST_DIAGSENDDATAV0_H
#define DLLTEST_DIAGSENDDATAV0_H

#include "../entity/Diag.h"

// 诊断状态固定为这四个状态，不再增加，失败原因将通过errorStatus来标识
enum DiagSessionState {
//    发送未完成
    sendUnfinished,
//    发送完成
    sendComplete,
//    接收完成
    received,
//    失败
    failed,
};
//寻址方式，物理地址或功能地址
enum AddressingMode {
    physical = 0,
    functional = 1,
};
// 异常状态
enum ErrorStatus {
//    暂定长度为4个字节，每个字节的一个bit位表示一个异常,总共可以表示32个异常
//    发送超时
    SendTimeout = 0x1,
//    响应超时
    ResponseTimeout = 0x2,
//    流控帧超时
    BsTimeout = 0x4,
//    流控帧错误
    FlowControlError = 0x8,
    flowControlOverflow = 0x10,
};
typedef struct DiagSession {
    uint32_t id;
    AddressingMode addressingMode = physical;
    DiagSessionState diagSessionState;  // 状态
    uint32 errorStatus;  // 异常状态 ErrorStatus
    std::vector<cclCanMessage *> sendData;   // 已发送的数据
    std::vector<cclCanMessage *> receiveData; // 已接收的数据
    uint32_t dataLength = 0;
    uint8_t *data = nullptr;
    bool parsed = false;// 解析是否完成？
    uint32_t offset = 0;// 偏移量
    uint8_t SN = 0;// 连续帧序号

//    设置errorStatus
    void setErrorStatus(ErrorStatus status) {
        this->errorStatus |= status;
    }

//    获取errorStatus
    bool getErrorStatus(ErrorStatus status) {
        return this->errorStatus & status;
    }
} DiagSession;
#endif //DLLTEST_DIAGSENDDATAV0_H
