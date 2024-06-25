//
// Created by fanshuhua on 2024/6/17.
//

#ifndef DLLTEST_EVENTMULTICASTER_H
#define DLLTEST_EVENTMULTICASTER_H

#include "EventListener.h"

/*
 * EventMulticaster  事件广播器
 * */
class EventMulticaster {
private:
    std::vector<EventListener *> listeners;
public:
    static EventMulticaster *getInstance() {
        static EventMulticaster *instance = nullptr;
        if (instance == nullptr) {
            instance = new EventMulticaster();
        }
        return instance;
    }

    void addListener(EventListener *listener);

    void removeListener(EventListener *listener);

    void notify(EventType type, void *event);
};


#endif //DLLTEST_EVENTMULTICASTER_H
