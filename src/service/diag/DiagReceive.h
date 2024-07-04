//
// Created by fanshuhua on 2024/7/3.
//

#ifndef DLLTEST_DIAGRECEIVE_H
#define DLLTEST_DIAGRECEIVE_H


class DiagReceive : public EventListener {
private:
    Node *node;
    DiagEventMulticaster *diagEventMulticaster;
public:
    explicit DiagReceive(Node *node) {
        this->node = node;
        this->diagEventMulticaster = node->diagConfig->diagEventMulticaster;
    }

    bool onEvent(EventType type, void *event) override;

    void callback(void *event) override;
};


#endif //DLLTEST_DIAGRECEIVE_H
