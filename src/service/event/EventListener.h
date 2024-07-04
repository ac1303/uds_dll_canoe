//
// Created by fanshuhua on 2024/6/17.
//

#ifndef DLLTEST_EVENTLISTENER_H
#define DLLTEST_EVENTLISTENER_H
enum EventType {
    TimeEvent,
    CanEvent,
    VarEvent,

    DiagAddSessionEvent, // 添加诊断请求
    DiagStartSessionEvent,//    诊断请求开始
    DiagEndSessionEvent,//    诊断请求结束
    DiagWaitFlowControlEvent,//    等待流控帧事件
    DiagReceiveFlowControlEvent,//    接收到流控帧事件
    DiagReceiveResponseEvent,//    接收到响应帧事件
};

class EventListener {
public:
    virtual bool onEvent(EventType type, void *event) = 0;

    virtual void callback(void *event) = 0;
};


#endif //DLLTEST_EVENTLISTENER_H
