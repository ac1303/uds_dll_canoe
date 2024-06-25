//
// Created by fanshuhua on 2024/6/17.
//

#include "EventMulticaster.h"

void EventMulticaster::addListener(EventListener *listener) {
    listeners.push_back(listener);
}

void EventMulticaster::removeListener(EventListener *listener) {
    for (auto it = listeners.begin(); it != listeners.end(); it++) {
        if (*it == listener) {
            listeners.erase(it);
            break;
        }
    }
}

void EventMulticaster::notify(EventType type, void *event) {
    for (auto listener: listeners) {
        if (listener->onEvent(type, event)) {
            listener->run();
        }
    }
}
