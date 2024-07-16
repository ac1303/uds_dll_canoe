//
// Created by fanshuhua on 2024/7/3.
//

#ifndef DLLTEST_DIAGSEND_H
#define DLLTEST_DIAGSEND_H

#include "DiagEventMulticaster.cpp"

enum DiagSendStatus {
    DiagSendStatus_IDLE = 0,
    DiagSendStatus_SENDING = 1,
};
class DiagSend : public EventListener {
private:
// 是否满足发送条件
    struct SendCondition {
        bool sendSuccess = true;//    上一帧发送成功
        bool stMin = true;//        STmin 连续帧间隔 满足
        bool delayTime = true; //        延时时间满足
        bool flowControlFrame = true;  //        流控帧满足
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
    SendCondition sendCondition = SendCondition();

    Node *node;
    std::queue<DiagSession *> diagSessionQueue;
    DiagSendStatus status = DiagSendStatus_IDLE;
    DiagEventMulticaster *diagEventMulticaster = nullptr;


//    判断流控帧
    bool judgeFlowControlFrame(cclCanMessage *canMessage);

//    发送失败结束
    void sendFailed(DiagSessionState diagSessionState, ErrorStatus errorStatus);
public:
    explicit DiagSend(Node *node) {
        this->node = node;
        diagEventMulticaster = this->node->diagConfig->diagEventMulticaster;
        diagEventMulticaster->addListener(this);
    }
    bool onEvent(EventType type, void *event) override;
    void callback(void *event) override;

    void addSession(DiagSession *session);
};


#endif //DLLTEST_DIAGSEND_H
