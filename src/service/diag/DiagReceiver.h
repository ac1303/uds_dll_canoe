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
    Node *node;
public:
    DiagReceiver(Node *node);
    bool onEvent(EventType type, void *event) override;
    void run() override;

    ~DiagReceiver() {
        EventMulticaster::getInstance()->removeListener(this);
    }
};


#endif //DLLTEST_DIAGRECEIVER_H
