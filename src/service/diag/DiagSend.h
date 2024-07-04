//
// Created by fanshuhua on 2024/7/3.
//

#ifndef DLLTEST_DIAGSEND_H
#define DLLTEST_DIAGSEND_H

#include "DiagEventMulticaster.cpp"

enum DiagSendStatus {
    DiagSendStatus_IDLE = 0,
    DiagSendStatus_SENDING = 1,
};
class DiagSend : public EventListener {
private:
    Node *node;
    std::queue<DiagSession *> diagSessionQueue;
    DiagSendStatus status = DiagSendStatus_IDLE;
    DiagEventMulticaster *diagEventMulticaster = nullptr;

    void addSession(DiagSession *session);
public:
    explicit DiagSend(Node *node) {
        this->node = node;
        diagEventMulticaster = this->node->diagConfig->diagEventMulticaster;
        diagEventMulticaster->addListener(this);
    }
    bool onEvent(EventType type, void *event) override;
    void callback(void *event) override;
};


#endif //DLLTEST_DIAGSEND_H
