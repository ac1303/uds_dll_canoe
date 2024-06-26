//
// Created by fanshuhua on 2024/6/17.
//

#ifndef DLLTEST_DIAGTRANSMITTER_H
#define DLLTEST_DIAGTRANSMITTER_H

#include "DiagParsing.h"
#include "DiagParsing.cpp"
#include "../event/EventListener.h"
#include "../event/EventMulticaster.h"
#include "../../model/entity/Node.h"

/*
 * 诊断发送器，构造函数中传入诊断数据，然后进行发送，
 * 如果多帧发送，则添加定时器，定时发送
 * */


class DiagTransmitter : public EventListener {
private:

// 是否满足发送条件
    struct SendCondition {
        bool sendSuccess = true;//    上一帧发送成功
        bool stMin = true;//        STmin 连续帧间隔 满足
        bool delayTime = true; //        延时时间满足
        bool flowControlFrame = true;

//        判断条件是否满足
        [[nodiscard]] bool isSendCondition() const {
            return sendSuccess
                   && stMin
                   && delayTime
                   && flowControlFrame;
        }
    };

//  距离下一次等待流控帧还差几帧
    int flowControlFrameCount = 1;
    uint8_t Stmin = 0;
    std::shared_ptr<cclCanMessage> flowControlFrame = nullptr;

    DiagSession *parsingDTO;
    Node *node;
    SendCondition *sendCondition;

    bool sendSuccess(cclCanMessage *message);

    bool waitFlowControlFrame(cclCanMessage *message);

    bool onCanEvent(EventType type, cclCanMessage *message);

    bool sendTimeout(long long time);

    bool stMinTimeout(long long time);

    bool waitFlowControlFrameTimeout(long long time);

    bool onTimeEvent(EventType type, long long time);

public:
    explicit DiagTransmitter(DiagSession *parsingDTO, Node *node);

    bool onEvent(EventType type, void *event) override;

    void run() override;

    ~DiagTransmitter() {
        EventMulticaster::getInstance()->removeListener(this);
    }
};


#endif //DLLTEST_DIAGTRANSMITTER_H
