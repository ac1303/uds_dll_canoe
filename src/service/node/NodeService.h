//
// Created by fanshuhua on 2024/6/25.
//

#ifndef DLLTEST_NODESERVICE_H
#define DLLTEST_NODESERVICE_H

//全局变量用于存储节点
static std::map<uint16_t, Node *> nodeMap = std::map<uint16_t, Node *>();

class NodeService {
private:
public:
    static int8_t createNode(uint16_t nmId);
};


#endif //DLLTEST_NODESERVICE_H
