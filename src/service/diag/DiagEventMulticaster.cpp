//
// Created by fanshuhua on 2024/7/3.
//

#include "DiagEventMulticaster.h"

void DiagEventMulticaster::addListener(EventListener *listener) {
    listeners.push_back(listener);
}

void DiagEventMulticaster::removeListener(EventListener *listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void DiagEventMulticaster::notify(EventType type, void *event) {
    for (auto listener: listeners) {
        if (listener->onEvent(type, event)) {
            listener->callback(event);
        }
    }
}
