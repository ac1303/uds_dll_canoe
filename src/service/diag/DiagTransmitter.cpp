//
// Created by fanshuhua on 2024/6/17.
//

#include "DiagTransmitter.h"


DiagTransmitter::DiagTransmitter(DiagSession *parsingDTO, Node *node) {
    this->parsingDTO = parsingDTO;
    this->node = node;
    sendCondition = new SendCondition();
    EventMulticaster::getInstance()->addListener(this);
    DiagTransmitter::run();
}

void DiagTransmitter::run() {
    if (parsingDTO->parsed) {
        cclWrite("全部发送完成");
        parsingDTO->diagSessionState = sendComplete;
        DiagTransmitter::~DiagTransmitter();
        return;
    }
    if (!sendCondition->isSendCondition()) {
        return;
    }
    if (flowControlFrameCount == 0) {
        sendCondition->flowControlFrame = false;
        return;
    }
    cclCanMessage *message = ParsingFactory::getInstance()->parse(parsingDTO, node->diagConfig);
    if (message != nullptr) {
        message->time = globalVar.runTime;
        message->channel = globalVar.VIAChannel;
        message->dir = kVIA_Tx;
        globalVar.canBus->OutputMessage3(message->channel, message->id, message->flags, 0  // 重发次数
                , message->dataLength, message->data);
    }
    sendCondition->sendSuccess = false;
    sendCondition->stMin = false;
    flowControlFrameCount--;
    if (flowControlFrameCount == 0) {
        sendCondition->flowControlFrame = false;
    }
    parsingDTO->sendData.push_back(message);
}

bool DiagTransmitter::onEvent(EventType type, void *event) {
    switch (type) {
        case CanEvent:
            return onCanEvent(type, static_cast<cclCanMessage *>(event));
        case TimeEvent:
            return onTimeEvent(type, *static_cast<long long int *>(event));
        default:
            return false;
    }
}

// 判断是否发送成功
bool DiagTransmitter::sendSuccess(cclCanMessage *message) {
    if (sendCondition->sendSuccess) {
        return false;
    }
    cclCanMessage lastMessage = *parsingDTO->sendData.back();
    if (message->operator==(lastMessage)) {
        sendCondition->sendSuccess = true;
//        更新发送成功时间
        parsingDTO->sendData.back()->time = message->time;
        return true;
    }
    return false;
}

bool DiagTransmitter::waitFlowControlFrame(cclCanMessage *message) {
    if (sendCondition->flowControlFrame) {
        return false;
    }
    if (message->id != node->diagConfig->RespAddr) {
        return false;
    }
    if ((message->data[0] & 0xF0) != 0x30) {
        return false;
    }
    int flowControlStatus = message->data[0] & 0x0F;
    flowControlFrame = std::make_shared<cclCanMessage>(*message);
    if (flowControlStatus == 0) {
        sendCondition->flowControlFrame = true;
        sendCondition->stMin = false;
        flowControlFrameCount = message->data[1];
        Stmin = message->data[2];
        return true;
    }
    if (flowControlStatus == 1) {
        sendCondition->flowControlFrame = false;
//        TODO 暂时不做处理,等以后再说
        cclPrintf("流控帧状态为等待,暂时不做处理!!!");
        parsingDTO->diagSessionState = sendFailed;
        DiagTransmitter::~DiagTransmitter();
        return false;
    }
    if (flowControlStatus == 2) {
        parsingDTO->diagSessionState = flowControlOverflow;
        cclPrintf("流控帧溢出");
        parsingDTO->diagSessionState = sendFailed;
        DiagTransmitter::~DiagTransmitter();
        return false;
    }
    parsingDTO->diagSessionState = flowControlError;
    parsingDTO->diagSessionState = sendFailed;
    cclPrintf("异常流控帧");
    DiagTransmitter::~DiagTransmitter();
    return false;
}

bool DiagTransmitter::onCanEvent(EventType type, cclCanMessage *message) {
    if (type != CanEvent) {
        return false;
    }
    return sendSuccess(message) || waitFlowControlFrame(message);
}

// ========================================================================
bool DiagTransmitter::sendTimeout(long long int time) {
    if (sendCondition->sendSuccess) {
        return false;
    }
    long long int lastTime = parsingDTO->sendData.back()->time;
    if (time - lastTime >= cclTimeMilliseconds(node->diagConfig->networkLayerTime->N_As)) {
        sendCondition->sendSuccess = false;
        parsingDTO->diagSessionState = sendFailed;
        cclPrintf("发送失败，发送超时");
        DiagTransmitter::~DiagTransmitter();
    }
    return false;
}

bool DiagTransmitter::waitFlowControlFrameTimeout(long long int time) {
    if (sendCondition->flowControlFrame) {
        return false;
    }
    long long int N_Bs = cclTimeMilliseconds(node->diagConfig->networkLayerTime->N_Bs);
    if (time - parsingDTO->sendData.back()->time > N_Bs) {
        parsingDTO->diagSessionState = BsTimeout;
        parsingDTO->diagSessionState = sendFailed;
        cclPrintf("DiagTransmitter::waitFlowControlFrameTimeout   等待流控帧超时");
        DiagTransmitter::~DiagTransmitter();
        return true;
    }
    return false;
}

bool DiagTransmitter::stMinTimeout(long long int time) {
    if (sendCondition->stMin) {
        return false;
    }
    long long int stmin = cclTimeMilliseconds(Stmin);
    long long int lastTime = parsingDTO->sendData.back()->time;
    if (flowControlFrame != nullptr) {
        lastTime = flowControlFrame->time > lastTime ? flowControlFrame->time : lastTime;
    }
    if (time - lastTime > stmin) {
        sendCondition->stMin = true;
        return true;
    }
    return false;
}

bool DiagTransmitter::onTimeEvent(EventType type, long long int time) {
    if (type != TimeEvent) {
        return false;
    }
    sendTimeout(time);
    return stMinTimeout(time) || waitFlowControlFrameTimeout(time);
}



