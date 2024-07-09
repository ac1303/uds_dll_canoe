//
// Created by fanshuhua on 2024/7/3.
//

#ifndef DLLTEST_DIAGRECEIVE_H
#define DLLTEST_DIAGRECEIVE_H


class DiagReceive : public EventListener {
private:
    Node *node;
    DiagEventMulticaster *diagEventMulticaster;

//    判断是否属于该节点的诊断报文
    bool isDiagMessage(cclCanMessage *canMessage);

//    处理物理寻址报文
    void handlePhyAddr(cclCanMessage *canMessage);

//    处理功能寻址报文
    void handleFuncAddr(cclCanMessage *canMessage);

//    处理响应报文
    void handleRespAddr(cclCanMessage *canMessage);
public:
    DiagReceive(Node *node) {
        this->node = node;
        this->diagEventMulticaster = node->diagConfig->diagEventMulticaster;
//        注册监听
        EventMulticaster::getInstance()->addListener(this);
    }

    bool onEvent(EventType type, void *event) override;

    void callback(void *event) override;
};


#endif //DLLTEST_DIAGRECEIVE_H
