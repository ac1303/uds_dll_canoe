//
// Created by 87837 on 2024/6/24.
//

#include "DiagReceiver.h"

DiagReceiver::DiagReceiver(DiagSession *session, Node *node) {
    this->session = session;
    this->node = node;
    EventMulticaster::getInstance()->addListener(this);
}

bool DiagReceiver::onEvent(EventType type, void *event) {
    return false;
}

void DiagReceiver::run() {

}
