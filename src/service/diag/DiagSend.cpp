//
// Created by fanshuhua on 2024/7/3.
//

#include "DiagSend.h"

bool DiagSend::onEvent(EventType type, void *event) {
    switch (type) {
        case DiagAddSessionEvent:
            addSession((DiagSession *) event);
            return false;
        case DiagStartSessionEvent:
            return true;
        case DiagReceiveFlowControlEvent:
//            接收到流控帧
            return DiagSend::judgeFlowControlFrame((cclCanMessage *) event);
        default:
            return false;
    }
}

void DiagSend::callback(void *event) {
    if (diagSessionQueue.empty()) {
        status = DiagSendStatus_IDLE;
        return;
    }
//    判断是否满足发送条件
    if (!sendCondition.isSendCondition()) {
        return;
    }
    status = DiagSendStatus_SENDING;
//    1. 取出队列中排在最前面的session
    DiagSession *session = diagSessionQueue.front();
//    2.进行解析
    if (session->parsed) {
//        如果解析完成，代表已经发送完成，则从发送队列中移除，并通知结束
        session->diagSessionState = DiagSessionState::sendUnfinished;
        diagSessionQueue.pop();
        diagEventMulticaster->notify(EventType::DiagEndSessionEvent, session);
        delete session;
        status = DiagSendStatus_IDLE;
        return;
    }
//    如果未解析完成，则进行解析
    cclCanMessage *message = ParsingFactory::getInstance()->parse(session, node->diagConfig);
//    3.发送数据
    if (message == nullptr) {
        session->setErrorStatus(ErrorStatus::UnknownError);
        diagEventMulticaster->notify(EventType::DiagEndSessionEvent, session);
        delete session;
        status = DiagSendStatus_IDLE;
        return;
    }
    message->time = globalVar.runTime;
    message->channel = globalVar.VIAChannel;
    message->dir = kVIA_Tx;
    globalVar.canBus->OutputMessage3(message->channel, message->id, message->flags, 0  // 重发次数
            , message->dataLength, message->data);
//    4.通知发送完成，等待判定发送是否成功
    session->sendData.push_back(message);
    sendCondition.sendSuccess = false;
    if (flowControlFrameCount > 0) {
        flowControlFrameCount--;
    }
    if (flowControlFrameCount == 0) {
        sendCondition.flowControlFrame = false;
    }
    diagEventMulticaster->notify(EventType::DiagSendWaitEvent, message);
}

void DiagSend::addSession(DiagSession *session) {
    diagSessionQueue.push(session);
    if (status == DiagSendStatus_IDLE) {
        diagEventMulticaster->notify(EventType::DiagStartSessionEvent, nullptr);
    }
}

bool DiagSend::judgeFlowControlFrame(cclCanMessage *canMessage) {
//    判断正常流控帧、等待流控帧、溢出流控帧和非流控帧
    switch (canMessage->data[0]) {
        case 0x30:
//            正常流控帧
            Stmin = canMessage->data[2];
            if (canMessage->data[1] == 0) {
//                如果不限制流控帧的数量，则设置为-1
                flowControlFrameCount = -1;
            } else {
                flowControlFrameCount = canMessage->data[1];
            }
            sendCondition.flowControlFrame = true;
            return true;
        case 0x31:
//            等待流控帧
            sendFailed(DiagSessionState::failed, ErrorStatus::flowControlOverflow);
//            TODO 收到等待流控帧，目前按失败处理，等待后续完善
            cclWrite("收到等待流控帧，目前按失败处理，等待后续完善");
            return false;
        case 0x32:
//            溢出流控帧
            sendFailed(DiagSessionState::failed, ErrorStatus::flowControlOverflow);
            return false;
        default:
//            非流控帧，设置状态后结束发送
            sendFailed(DiagSessionState::failed, ErrorStatus::FlowControlError);
            return false;
    }
}

void DiagSend::sendFailed(DiagSessionState diagSessionState, ErrorStatus errorStatus) {
    diagSessionQueue.front()->diagSessionState = diagSessionState;
    diagSessionQueue.front()->setErrorStatus(errorStatus);
    diagEventMulticaster->notify(EventType::DiagEndSessionEvent, diagSessionQueue.front());
    delete diagSessionQueue.front();
    diagSessionQueue.pop();
    status = DiagSendStatus_IDLE;
}
