//
// Created by fanshuhua on 2024/6/1.
//

#ifndef CAPLUTILS_CANMESSAGE_H
#define CAPLUTILS_CANMESSAGE_H

#include <vector>
#include "vector/CCL/CCL.h"
#include "../vo/DiagV0.h"


enum PaddingType {
    Padding = 0,        // 默认选择填充，DLC最小为8，当数据长度小于8时，填充CC
    NoPadding = 1,   // 即当数据类型小于8字节，不会进行填充，DLC为数据长度
};
typedef struct CanMessageConfig {
    bool RTR;
    bool Wakeup;
    bool TE;
    bool FDF;
    bool BRS;
    bool ESI;
} CanMessageConfig;

static std::map<uint8_t, uint8_t> DLC_AvailableLength = {{0,  0},
                                                         {1,  0},
                                                         {2,  1},
                                                         {3,  2},
                                                         {4,  3},
                                                         {5,  4},
                                                         {6,  5},
                                                         {7,  6},
                                                         {8,  7},
                                                         {9,  10},
                                                         {10, 14},
                                                         {11, 18},
                                                         {12, 22},
                                                         {13, 30},
                                                         {14, 46},
                                                         {15, 62}};
static std::map<uint8_t, uint8_t> DLC_ActualLength = {{0,  0},
                                                      {1,  1},
                                                      {2,  2},
                                                      {3,  3},
                                                      {4,  4},
                                                      {5,  5},
                                                      {6,  6},
                                                      {7,  7},
                                                      {8,  8},
                                                      {9,  12},
                                                      {10, 16},
                                                      {11, 20},
                                                      {12, 24},
                                                      {13, 32},
                                                      {14, 48},
                                                      {15, 64}};


// ISO 15765 网络层时间参数
typedef struct NetworkLayerTime {
    uint16_t N_As = 70;     // 发送方->接收方 发送方CAN报文发送时间，即首帧和连续帧在数据链路层传播的时间（参考简介中的图）。当N_As超时时，表示发送方没有及时发送N_PDU。
    uint16_t N_Ar = 70;     // 接收方->发送方 接收方CAN报文发送时间，即流控制帧在数据链路层传播的时间（参考简介中的图）。当N_Ar超时，表示接收方没有及时发送N_PDU
    uint16_t N_Bs = 1000;   // 发送方->接收方 直到下一个流控制帧接收的时间，即接收方收到首帧发出ACK响应，与自己（发送方）收到流控制帧的间隔时间,当N_Bs超时，表示发送方没有接收到流控帧。
    uint16_t N_Br = 70;     // 接收方->发送方 直到下一个流控制帧发送的时间，即自己（接收方）收到首帧，与自己开始发出流控制帧的间隔时间,当N_Br超时，表示接收方没有发出流控帧。
    uint16_t N_Cs = 70;     // 发送方->接收方 直到下一个连续帧发送的时间，即自己（发送方）收到流控制帧或者连续帧送达时产生的ACK响应，与自己开始发出新连续帧的时间间隔（参考简介中的图）；N_Cs不等于STmin
    uint16_t N_Cr = 1000;   // 接收方->发送方 直到下一个连续帧接收的时间，即自己（接收方）收到连续帧，到下一个自己接收到连续帧的时间间隔。当N_Cr超时：接收方没有收到连续帧。
} NetworkLayerTime;

// ISO 14229 会话层时间参数
typedef struct SessionLayerTime {
    uint8_t P2CanReq = 0;                          // P2CAN_Req的值 为向被寻址服务器发送请求的时间
    uint8_t P2CanResp = 0;                         // P2CAN_Rsp的值 为向客户端发送响应的时间
    uint16_t P2Can =
            P2CanReq + P2CanResp;          // P2CAN的值 分为向被寻址服务器发送请求的时间和向客户端发送响应的时间 P2CAN = P2CAN_Req + P2CAN_Rsp
    uint16_t P2Server = 50;                         // 服务器开始 响应 消息的运行需求，在接收到一个请求消息后（通过 N_USData.ind 指示）
    uint16_t P2ServerEx = 5000;                     // 服务器的开始 响应 消息的运行需求，在它传输一个响应代码为78 hex（增强响应时序） 的否定响应消息（通过 N_USData.con 指示）后
    uint16_t P2Client = static_cast<uint16_t>(P2Server +
                                              P2Can);             // 客户端在成功传输请求消息后（通过N_USData.con指示）等待响应消息的开始传入（多帧消息的N_USDataFirstFrame.ind或单帧 消息的N_USData.ind）所引起的超时
    uint16_t P2ClientEx = static_cast<uint16_t>(P2ServerEx +
                                                P2CanResp);     // 客户端在接收到应答码为 78 hex 的否定响应后（通过N_USData.ind 指示）等待接收响应消息（多帧消息的N_USDataFirstFrame.ind或单帧 消息的N_USData.ind）所引起的增强超时
    uint16_t P2ClientPhys = 0;               // 客户端在成功发送无需任何响应物理寻址请求消息（通过N_USData.con指示）之后，可以传输下一个物理寻址请求消息（请参见6.3.5.3）之前，最短等待时间。
    uint16_t P2ClientPhysEx = 0;           // 客户端在成功传输一个功能寻址请求消息到可以传输下一个功能寻址请求消息之间最短等待时间，此情况下不需要响应，或请求数据 只被允许使用子网级别的功能寻址服务
    uint16_t S3Client = 5000;                       // 客户端传输用于保持诊断会话的功能寻址TesterPresent（3E hex）请求消息时，消息之间间隔时间。而不是多服务器的默认会话活跃的时间（functional communication），或者向某个服务器物理传输的请求消息之间的最大时间（physical communication）
    uint16_t S3Server = 5000;                       // 服务器在不接收任何诊断请求消息的情况下保持除 defaultSession 之外的诊断会话处于活动状态的时间。
} SessionLayerTime;


// 流控帧配置
typedef struct FlowControlFrame {
    uint8_t FS = 0x30;
    uint8_t BS = 0x00;
    uint8_t STmin = 0x00;
} FlowControlFrame;

typedef struct DiagConfig {
    uint16_t PhyAddr = 0x73A;
    uint16_t FuncAddr = 0x7DF;
    uint16_t RespAddr = 0x7BA;
    NetworkLayerTime *networkLayerTime = new NetworkLayerTime();
    SessionLayerTime *sessionLayerTime = new SessionLayerTime();
    uint8_t maxDLC = 8;
    PaddingType paddingType = Padding;
    uint8_t paddingData = 0xCC;
    CanMessageConfig canMessageConfig = {
            .RTR = false,
            .Wakeup = false,
            .TE = false,
            .FDF = true,
            .BRS = true,
            .ESI = false
    };
    FlowControlFrame *flowControlFrame = new FlowControlFrame();
//    容错时间，当规范时间>实际时间>规范时间+容错时间时，依然可以接收到数据，单位ms
    uint16_t faultToleranceTime = 100;
} DiagConfig;

#endif //CAPLUTILS_CANMESSAGE_H
