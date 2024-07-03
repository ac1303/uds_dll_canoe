//
// Created by fanshuhua on 2024/7/3.
//

#include "DiagSend.h"

bool DiagSend::onEvent(EventType type, void *event) {
    if (type == DiagAddSessionEvent) {
        return true;
    }
}

void DiagSend::callback(void *event) {
}