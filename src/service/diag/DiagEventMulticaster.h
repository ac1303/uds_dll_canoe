//
// Created by fanshuhua on 2024/7/3.
//

#ifndef DLLTEST_DIAGEVENTMULTICASTER_H
#define DLLTEST_DIAGEVENTMULTICASTER_H

#include "../event/EventListener.h"

class DiagEventMulticaster {
private:
    std::vector<EventListener *> listeners;
public:
    void addListener(EventListener *listener);

    void removeListener(EventListener *listener);

    void notify(EventType type, void *event);
};


#endif //DLLTEST_DIAGEVENTMULTICASTER_H
