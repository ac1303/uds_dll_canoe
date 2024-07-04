//
// Created by fanshuhua on 2024/7/3.
//

#include "DiagSend.h"

bool DiagSend::onEvent(EventType type, void *event) {
    switch (type) {
        case DiagAddSessionEvent:
            return false;
        case DiagStartSessionEvent:
            return true;
        default:
            return false;
    }
}

void DiagSend::callback(void *event) {
    if (diagSessionQueue.empty()) {
        status = DiagSendStatus_IDLE;
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
    if (message == nullptr) {
        session->setErrorStatus(ErrorStatus::UnknownError);
        diagEventMulticaster->notify(EventType::DiagEndSessionEvent, session);
        delete session;
        status = DiagSendStatus_IDLE;
        return;
    }
//    3.发送数据




//    diagEventMulticaster->notify(EventType::DiagEndSessionEvent, session);
//    delete session;
//    status = DiagSendStatus_IDLE;
}

void DiagSend::addSession(DiagSession *session) {
    diagSessionQueue.push(session);
    if (status == DiagSendStatus_IDLE) {
        diagEventMulticaster->notify(EventType::DiagStartSessionEvent, nullptr);
    }
}
