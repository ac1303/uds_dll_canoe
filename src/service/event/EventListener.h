//
// Created by fanshuhua on 2024/6/17.
//

#ifndef DLLTEST_EVENTLISTENER_H
#define DLLTEST_EVENTLISTENER_H
enum EventType {
    TimeEvent,
    CanEvent,
    VarEvent
};

class EventListener {
public:
    virtual bool onEvent(EventType type, void *event) = 0;

    virtual void run() = 0;
};


#endif //DLLTEST_EVENTLISTENER_H
