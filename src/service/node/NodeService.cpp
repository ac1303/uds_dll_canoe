//
// Created by fanshuhua on 2024/6/25.
//

#include "NodeService.h"
#include "../diag/DiagSend.h"


int8_t NodeService::createNode(uint16_t nmId) {
    if (nodeMap.find(nmId) != nodeMap.end()) {
        return 0;
    }
    Node *node = new Node();
    node->NodeHandle = nmId;
    node->BaseId = nmId & 0xFF00;
    node->EcuId = nmId & 0x00FF;
    nodeMap.insert(std::pair<uint16_t, Node *>(nmId, node));
    return 1;
}
