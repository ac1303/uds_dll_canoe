//
// Created by 87837 on 2024/6/11.
//

#include "DiagParsing.h"

uint32_t createFlag(CanMessageConfig canMessageConfig) {
    uint32_t flags =
            (canMessageConfig.RTR ? kVIA_CAN_RemoteFrame : 0) |
            (canMessageConfig.Wakeup ? kVIA_CAN_Wakeup : 0) |
            (canMessageConfig.TE ? kVIA_CAN_NERR : 0) |
            (canMessageConfig.FDF ? kVIA_CAN_EDL : 0) |
            (canMessageConfig.BRS ? kVIA_CAN_BRS : 0) |
            (canMessageConfig.ESI ? kVIA_CAN_ESI : 0);
    return flags;
}

uint8_t getDLC(uint32_t dataLength) {
//    判断dataLength处于哪个区间，遍历DLC_MAPPING
    for (int i = 2; i <= 15; ++i) {
        if (dataLength <= DLC_AvailableLength[i]) {
            return i;
        }
    }
    cclPrintf("%s %d: 数据长度超过DLC最大值", __func__, __LINE__);
    return 0;
}

bool copy(uint8_t dest[], uint8_t offset, uint8_t src[], uint8_t length, uint8_t paddingData) {
    if (length > 64) {
        cclPrintf("数据长度超过64，不应该进入copy");
        return false;
    }
//    数据全部填充paddingData
    memset(dest + offset, paddingData, 64 - offset);
    uint32_t result = memcpy_s(dest + offset, 64 - offset, src, length);
    return result == 0;
}

bool SF_Parsing::isSupport(DiagSession *parsingDTO, DiagConfig *diagConfig) {
    return parsingDTO->dataLength <= DLC_AvailableLength[diagConfig->maxDLC]
           && parsingDTO->offset == 0
           && !parsingDTO->parsed;
}

cclCanMessage *SF_Parsing::parse(DiagSession *parsingDTO, DiagConfig *diagConfig) {
    if (parsingDTO->dataLength > DLC_AvailableLength[diagConfig->maxDLC]) {
        cclPrintf("数据长度超过DLC最大值，不应该进入SF_Parsing");
        parsingDTO->parsed = true;
        return {};
    }
    auto *canMessageVO = new cclCanMessage();
    canMessageVO->id = parsingDTO->addressingMode == physical ? diagConfig->PhyAddr : diagConfig->FuncAddr;
    canMessageVO->flags = createFlag(diagConfig->canMessageConfig);
    // 处理长度小于8的情况
    if (parsingDTO->dataLength < 8) {
        canMessageVO->dataLength = parsingDTO->dataLength + 1;
        canMessageVO->data[0] = parsingDTO->dataLength;
        copy(canMessageVO->data, 1, parsingDTO->data, parsingDTO->dataLength, diagConfig->paddingData);
    } else {
//        1、获取应该使用的dlc
        canMessageVO->dataLength = DLC_ActualLength[getDLC(parsingDTO->dataLength)];
//        2、填充数据
        canMessageVO->data[0] = 0;
        canMessageVO->data[1] = parsingDTO->dataLength;
        copy(canMessageVO->data, 2, parsingDTO->data, parsingDTO->dataLength, diagConfig->paddingData);
    }
    if (diagConfig->paddingType == Padding && parsingDTO->dataLength < 8) {
        canMessageVO->dataLength = 8;
    }
    parsingDTO->offset = parsingDTO->dataLength;
    parsingDTO->parsed = true;
    return canMessageVO;
}

bool FF_Parsing::isSupport(DiagSession *parsingDTO, DiagConfig *diagConfig) {
    return parsingDTO->dataLength > DLC_AvailableLength[diagConfig->maxDLC]
           && parsingDTO->offset == 0
           && !parsingDTO->parsed;
}

cclCanMessage *FF_Parsing::parse(DiagSession *parsingDTO, DiagConfig *diagConfig) {
    if (parsingDTO->dataLength <= DLC_AvailableLength[diagConfig->maxDLC]) {
        cclPrintf("数据长度小于DLC最大值，不应该进入FF_Parsing");
        parsingDTO->parsed = true;
        return {};
    }
    if (diagConfig->maxDLC < 8) {
        cclPrintf("DLC最大值小于8，不应该进入FF_Parsing");
        parsingDTO->parsed = true;
        return {};
    }
    // 1、生成首帧
    auto *FF = new cclCanMessage();
    FF->id = parsingDTO->addressingMode == physical ? diagConfig->PhyAddr : diagConfig->FuncAddr;
    FF->flags = createFlag(diagConfig->canMessageConfig);
    FF->dataLength = DLC_ActualLength[diagConfig->maxDLC];
    uint32_t offset = 2;
    if (parsingDTO->dataLength <= 4095) {
        FF->data[0] = 0x10 | (parsingDTO->dataLength >> 8);
        FF->data[1] = parsingDTO->dataLength & 0xFF;
        offset = 2;
    } else {
        FF->data[0] = 0x10;
        FF->data[1] = 0x00;
        FF->data[2] = (parsingDTO->dataLength >> 24) & 0xFF;
        FF->data[3] = (parsingDTO->dataLength >> 16) & 0xFF;
        FF->data[4] = (parsingDTO->dataLength >> 8) & 0xFF;
        FF->data[5] = parsingDTO->dataLength & 0xFF;
        offset = 6;
    }
    // 首帧可填充数据长度
    uint8_t FFDataLength = DLC_ActualLength[diagConfig->maxDLC] - offset;
    bool result = copy(FF->data, offset, parsingDTO->data, FFDataLength, diagConfig->paddingData);
    parsingDTO->offset = FFDataLength;
    parsingDTO->parsed = false;
    return result ? FF : nullptr;
}

bool CF_Parsing::isSupport(DiagSession *parsingDTO, DiagConfig *diagConfig) {
    return parsingDTO->dataLength > DLC_AvailableLength[diagConfig->maxDLC]
           && parsingDTO->offset > 0
           && !parsingDTO->parsed;
}

cclCanMessage *CF_Parsing::parse(DiagSession *parsingDTO, DiagConfig *diagConfig) {
    if (parsingDTO->offset >= parsingDTO->dataLength) {
        parsingDTO->parsed = true;
        return {};
    }
    auto *CF = new cclCanMessage();
    CF->id = parsingDTO->addressingMode == physical ? diagConfig->PhyAddr : diagConfig->FuncAddr;
    CF->flags = createFlag(diagConfig->canMessageConfig);
    if (parsingDTO->dataLength - parsingDTO->offset > DLC_ActualLength[diagConfig->maxDLC] - 1) {
        CF->dataLength = DLC_ActualLength[diagConfig->maxDLC];
    } else {
        for (int i = 2; i <= diagConfig->maxDLC; ++i) {
            if (parsingDTO->dataLength - parsingDTO->offset <= DLC_ActualLength[i] - 1) {
                CF->dataLength = DLC_ActualLength[i];
                parsingDTO->parsed = true;
                break;
            }
        }
    }
    // SN 先自增1
    CF->data[0] = 0x20 | (++parsingDTO->SN & 0x0F);
    bool result = copy(CF->data, 1, parsingDTO->data + parsingDTO->offset, CF->dataLength - 1, diagConfig->paddingData);
    parsingDTO->offset += CF->dataLength - 1;
    if (CF->dataLength < 8 && diagConfig->paddingType == Padding) {
        CF->dataLength = 8;
    }
    return result ? CF : nullptr;
}