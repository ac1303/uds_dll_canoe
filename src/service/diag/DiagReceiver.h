//
// Created by 87837 on 2024/6/24.
//

#ifndef DLLTEST_DIAGRECEIVER_H
#define DLLTEST_DIAGRECEIVER_H


#include "../event/EventListener.h"
#include "../event/EventMulticaster.h"
#include "../../model/entity/Node.h"

class DiagReceiver : public EventListener {
private:
    DiagSession *session;
    Node *node;

public:
    DiagReceiver(DiagSession *session, Node *node);

    bool onEvent(EventType type, void *event) override;

    void run() override;
};


#endif //DLLTEST_DIAGRECEIVER_H
