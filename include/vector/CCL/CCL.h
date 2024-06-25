/* ====================================================================================================================
File Name     CCL/CCL.h
Project       CANalyzer/CANoe C Library API (CCL)
Revision      2.2
Date          2021-12-14
Copyright (c) Vector Informatik GmbH. All rights reserved.

Description
This header file declares the API of an CANalyzer/CANoe C Library (CCL).

History
1.0  2011-01-24  First version of the API created.
1.1  2011-02-11  cclPrintf added
1.2  2014-04-04  Signal access added
1.3  2014-06-12  cclGetUserFilePath added
1.4  2014-06-25  Structures for system variables supported
1.5  2014-12-09  cclSysVarGetType added
1.6  2015-08-15  System variable (struct members) physical values supported
1.7  2017-04-21  Function Bus API added, upgrade to VS 2013
1.8  2017-06-28  System variable Integer64 values supported
1.9  2017-10-16  Function Bus Service Field support
1.10 2018-09-25  cclCanGetChannelNumber and cclLinGetChannelNumber added
1.11 2019-07-03  Support for CAN FD added
2.0  2021-01-15  For portability reasons the API now uses the new types of the C99 Standard declared in <stdint.h>
                 Support for platforms Windows-x64 and Linux-x64 added
                 Support for the compilers Clang, GCC, Mingw-w64 added
                 Visual Studio project files upgraded to VS 2017, CMake project files added
                 Support for Distributed Objects added
2.1  2021-07-16  Support for Ethernet frames (only Simulated, only channel-based) added
                 Support for Slave Mode (increment simulation time)
                 Added handler for LIN slave response changed
2.2  2021-12-14  LIN functions for sleep and wakeup handling added
==================================================================================================================== */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif

// ============================================================================
// Error Codes
// ============================================================================

#define CCL_SUCCESS                       ((int32_t)   0)
#define CCL_WRONGSTATE                    ((int32_t)  -1)
#define CCL_INTERNALERROR                 ((int32_t)  -2)
#define CCL_INVALIDCHANNEL                ((int32_t)  -3)
#define CCL_INVALIDTIMERID                ((int32_t)  -4)
#define CCL_INVALIDFUNCTIONPOINTER        ((int32_t)  -5)
#define CCL_INVALIDTIME                   ((int32_t)  -6)
#define CCL_INVALIDNAME                   ((int32_t)  -7)
#define CCL_SYSVARNOTDEFINED              ((int32_t)  -8)
#define CCL_INVALIDSYSVARID               ((int32_t)  -9)
#define CCL_WRONGTYPE                     ((int32_t) -10)
#define CCL_SYSVARISREADONLY              ((int32_t) -11)
#define CCL_BUFFERTOSMALL                 ((int32_t) -12)
#define CCL_WRONGARRAYSIZE                ((int32_t) -13)
#define CCL_NOTIMPLEMENTED                ((int32_t) -14)
#define CCL_SIGNALNOTDEFINED              ((int32_t) -15)
#define CCL_SIGNALAMBIGUOUS               ((int32_t) -16)
#define CCL_INVALIDSIGNALID               ((int32_t) -17)
#define CCL_USERFILENOTDEFINED            ((int32_t) -18)
#define CCL_PARAMETERINVALID              ((int32_t) -19)
#define CCL_INVALIDPORTPATH               ((int32_t) -20)
#define CCL_INVALIDMEMBERPATH             ((int32_t) -21)
#define CCL_INVALIDVALUEID                ((int32_t) -23)
#define CCL_INVALIDFUNCTIONPATH           ((int32_t) -24)
#define CCL_INVALIDFUNCTIONID             ((int32_t) -25)
#define CCL_INVALIDCALLCONTEXTID          ((int32_t) -26)
#define CCL_INVALIDSERVICEPATH            ((int32_t) -27)
#define CCL_INVALIDSERVICEID              ((int32_t) -28)
#define CCL_INVALIDADDRESSID              ((int32_t) -29)
#define CCL_INVALIDCONSUMERPATH           ((int32_t) -30)
#define CCL_INVALIDCONSUMERID             ((int32_t) -31)
#define CCL_INVALIDPROVIDERPATH           ((int32_t) -32)
#define CCL_INVALIDPROVIDERID             ((int32_t) -33)
#define CCL_INVALIDCONSUMEDSERVICEPATH    ((int32_t) -34)
#define CCL_INVALIDCONSUMEDSERVICEID      ((int32_t) -35)
#define CCL_INVALIDPROVIDEDSERVICEPATH    ((int32_t) -36)
#define CCL_INVALIDPROVIDEDSERVICEID      ((int32_t) -37)
#define CCL_INVALIDPDUSENDERPATH          ((int32_t) -38)
#define CCL_INVALIDPDUSENDERID            ((int32_t) -39)
#define CCL_INVALIDPDURECEIVERPATH        ((int32_t) -40)
#define CCL_INVALIDPDURECEIVERID          ((int32_t) -41)
#define CCL_INVALIDCONSUMEDPDUPATH        ((int32_t) -42)
#define CCL_INVALIDCONSUMEDPDUID          ((int32_t) -43)
#define CCL_INVALIDPROVIDEDPDUPATH        ((int32_t) -44)
#define CCL_INVALIDPROVIDEDPDUID          ((int32_t) -45)
#define CCL_INVALIDPDUPROVIDERPATH        ((int32_t) -46)
#define CCL_INVALIDPDUPROVIDERID          ((int32_t) -47)
#define CCL_INVALIDCONSUMEDEVENTPATH      ((int32_t) -48)
#define CCL_INVALIDCONSUMEDEVENTID        ((int32_t) -49)
#define CCL_INVALIDPROVIDEDEVENTPATH      ((int32_t) -50)
#define CCL_INVALIDPROVIDEDEVENTID        ((int32_t) -51)
#define CCL_INVALIDEVENTPROVIDERPATH      ((int32_t) -52)
#define CCL_INVALIDEVENTPROVIDERID        ((int32_t) -53)
#define CCL_INVALIDCONSUMEDEVENTGROUPPATH ((int32_t) -54)
#define CCL_INVALIDCONSUMEDEVENTGROUPID   ((int32_t) -55)
#define CCL_INVALIDPROVIDEDEVENTGROUPPATH ((int32_t) -56)
#define CCL_INVALIDPROVIDEDEVENTGROUPID   ((int32_t) -57)
#define CCL_INVALIDEVENTGROUPPROVIDERPATH ((int32_t) -58)
#define CCL_INVALIDEVENTGROUPPROVIDERID   ((int32_t) -59)
#define CCL_TOOMANYCALLCONTEXTS           ((int32_t) -60)
#define CCL_INVALIDDOMAIN                 ((int32_t) -61)
#define CCL_VALUEREADONLY                 ((int32_t) -62)
#define CCL_VALUENOTOPTIONAL              ((int32_t) -63)
#define CCL_INVALIDSIDE                   ((int32_t) -64)
#define CCL_INVALIDCALLCONTEXTSTATE       ((int32_t) -65)
#define CCL_INVALIDBINDING                ((int32_t) -66)
#define CCL_CANNOTADDDYNAMICCONSUMER      ((int32_t) -67)
#define CCL_CANNOTADDDYNAMICPROVIDER      ((int32_t) -68)
#define CCL_CONSUMERNOTFOUND              ((int32_t) -69)
#define CCL_PROVIDERNOTFOUND              ((int32_t) -70)
#define CCL_NOTSUPPORTED                  ((int32_t) -71)
#define CCL_CALLCONTEXTEXPIRED            ((int32_t) -72)
#define CCL_VALUENOTACCESSIBLE            ((int32_t) -73)
#define CCL_STATICCONSUMER                ((int32_t) -74)
#define CCL_STATICPROVIDER                ((int32_t) -75)
#define CCL_CONSUMERNOTBOUND              ((int32_t) -76)
#define CCL_PROVIDERNOTBOUND              ((int32_t) -77)
#define CCL_CANNOTCREATEADDRESS           ((int32_t) -78)
#define CCL_WRONGBINDING                  ((int32_t) -79)
#define CCL_INVALIDDATA                   ((int32_t) -80)
#define CCL_ARRAYNOTRESIZABLE             ((int32_t) -81)
#define CCL_UNSUPPORTEDTYPE               ((int32_t) -82)
#define CCL_INVALIDSIGNALNAME             ((int32_t) -83)
#define CCL_INVALIDCONSUMEDFIELDPATH      ((int32_t) -84)
#define CCL_INVALIDPROVIDEDFIELDPATH      ((int32_t) -85)
#define CCL_INVALIDFIELDPROVIDERPATH      ((int32_t) -86)
#define CCL_FIELDNOTSUBSCRIBABLE          ((int32_t) -87)
#define CCL_INVALIDCONSUMEDFIELDID        ((int32_t) -88)
#define CCL_INVALIDPROVIDEDFIELDID        ((int32_t) -89)
#define CCL_INVALIDFIELDPROVIDERID        ((int32_t) -90)
#define CCL_PROVIDEDFIELDDISPOSED         ((int32_t) -91)
#define CCL_NETWORKNOTDEFINED             ((int32_t) -92)
#define CCL_NO_SYNCHRONOUS_CALL           ((int32_t) -93)

// ============================================================================
// Common defines 
// ============================================================================

#define CCL_DIR_RX    ((uint8_t) 0)
#define CCL_DIR_TX    ((uint8_t) 1)
#define CCL_DIR_TXRQ  ((uint8_t) 2)

// ============================================================================
// General Enumerations
// ============================================================================

typedef enum {
    CCL_COMMUNICATION_OBJECTS,
    CCL_DISTRIBUTED_OBJECTS
} cclDomain;

// ============================================================================
// General Functions
// ============================================================================

extern void cclOnDllLoad();

extern void cclSetMeasurementPreStartHandler(void (*function)());
extern void cclSetMeasurementStartHandler(void (*function)());
extern void cclSetMeasurementStopHandler(void (*function)());
extern void cclSetDllUnloadHandler(void (*function)());

extern void cclWrite(const char *text);
extern void cclPrintf(const char *format, ...);

extern int32_t cclGetUserFilePath(const char *filename,
                                  char *pathBuffer,
                                  int32_t pathBufferLength);

// ============================================================================
// Time & Timer 
// ============================================================================

extern int64_t cclTimeSeconds(int64_t seconds);
extern int64_t cclTimeMilliseconds(int64_t milliseconds);
extern int64_t cclTimeMicroseconds(int64_t microseconds);

extern int32_t cclTimerCreate(void (*function)(int64_t time, int32_t timerID));
extern int32_t cclTimerSet(int32_t timerID, int64_t nanoseconds);
extern int32_t cclTimerCancel(int32_t timerID);

extern int32_t cclIncrementTimerBase(int64_t newTimeBaseTicks, int32_t numberOfSteps);
extern int32_t cclIsRunningInSlaveMode(bool *isSlaveMode);

// ============================================================================
// System Variables
// ============================================================================

#define CCL_SYSVAR_INTEGER       ((int32_t) 1)
#define CCL_SYSVAR_FLOAT         ((int32_t) 2)
#define CCL_SYSVAR_STRING        ((int32_t) 3)
#define CCL_SYSVAR_DATA          ((int32_t) 4)
#define CCL_SYSVAR_INTEGERARRAY  ((int32_t) 5)
#define CCL_SYSVAR_FLOATARRAY    ((int32_t) 6)
#define CCL_SYSVAR_STRUCT        ((int32_t) 7)
#define CCL_SYSVAR_GENERICARRAY  ((int32_t) 8)

extern int32_t cclSysVarGetID(const char *systemVariablename);
extern int32_t cclSysVarGetType(int32_t sysVarID);
extern int32_t cclSysVarSetHandler(int32_t sysVarID,
                                   void (*function)(int64_t time, int32_t sysVarID));
extern int32_t cclSysVarGetArraySize(int32_t sysVarID);
extern int32_t cclSysVarSetInteger(int32_t sysVarID, int32_t x);
extern int32_t cclSysVarGetInteger(int32_t sysVarID, int32_t *x);
extern int32_t cclSysVarSetInteger64(int32_t sysVarID, int64_t x);
extern int32_t cclSysVarGetInteger64(int32_t sysVarID, int64_t *x);
extern int32_t cclSysVarSetFloat(int32_t sysVarID, double x);
extern int32_t cclSysVarGetFloat(int32_t sysVarID, double *x);
extern int32_t cclSysVarSetString(int32_t sysVarID, const char *text);
extern int32_t cclSysVarGetString(int32_t sysVarID, char *buffer, int32_t bufferLenght);
extern int32_t cclSysVarSetIntegerArray(int32_t sysVarID, const int32_t *x, int32_t length);
extern int32_t cclSysVarGetIntegerArray(int32_t sysVarID, int32_t *x, int32_t length);
extern int32_t cclSysVarSetFloatArray(int32_t sysVarID, const double *x, int32_t length);
extern int32_t cclSysVarGetFloatArray(int32_t sysVarID, double *x, int32_t length);
extern int32_t cclSysVarSetData(int32_t sysVarID, const uint8_t *x, int32_t length);
extern int32_t cclSysVarGetData(int32_t sysVarID, uint8_t *x, int32_t length);
extern int32_t cclSysVarSetPhysical(int32_t sysVarID, double x);
extern int32_t cclSysVarGetPhysical(int32_t sysVarID, double *x);

// ============================================================================
// Signal
// ============================================================================

extern int32_t cclSignalGetID(const char *signalname);
extern int32_t cclSignalSetHandler(int32_t signalID, void (*function)(int32_t signalID));

extern int32_t cclSignalGetRxPhysDouble(int32_t signalID, double *value);
extern int32_t cclSignalGetRxRawInteger(int32_t signalID, int64_t *value);
extern int32_t cclSignalGetRxRawDouble(int32_t signalID, double *value);

extern int32_t cclSignalSetTxPhysDouble(int32_t signalID, double value);
extern int32_t cclSignalSetTxRawInteger(int32_t signalID, int64_t value);
extern int32_t cclSignalSetTxRawDouble(int32_t signalID, double value);

extern int32_t cclSignalGetTxPhysDouble(int32_t signalID, double *value);
extern int32_t cclSignalGetTxRawInteger(int32_t signalID, int64_t *value);
extern int32_t cclSignalGetTxRawDouble(int32_t signalID, double *value);

// ============================================================================
// CAN 
// ============================================================================

#define CCL_CANFLAGS_RTR     ((uint32_t)  1)
#define CCL_CANFLAGS_WAKEUP  ((uint32_t)  2)
#define CCL_CANFLAGS_TE      ((uint32_t)  4)
#define CCL_CANFLAGS_FDF     ((uint32_t)  8)
#define CCL_CANFLAGS_BRS     ((uint32_t) 16)
#define CCL_CANFLAGS_ESI     ((uint32_t) 32)

extern int32_t cclCanOutputMessage(int32_t channel,
                                   uint32_t identifier,
                                   uint32_t flags,
                                   uint8_t dataLength,
                                   const uint8_t data[]);


#define CCL_CAN_ALLMESSAGES  ((uint32_t) 0xFFFFFFFF)

struct cclCanMessage {
    int64_t time;
    int32_t channel;
    uint32_t id;
    uint32_t flags;
    uint8_t dir;
    uint8_t dataLength;
    uint8_t data[64];

    bool operator==(const cclCanMessage &rhs) const {
//        if (time != rhs.time) return false;   建议不要比较时间，因为开始发送到实际发送成功的时间可能会有差异
        if (channel != rhs.channel) return false;
        if (id != rhs.id) return false;
        if (flags != rhs.flags) return false;
        if (dir != rhs.dir) return false;
        if (dataLength != rhs.dataLength) return false;
        for (int i = 0; i < dataLength; ++i) {
            if (data[i] != rhs.data[i]) return false;
        }
        return true;
    }
};

extern int32_t cclCanSetMessageHandler(int32_t channel,
                                       uint32_t identifier,
                                       void (*function)(struct cclCanMessage *message));

extern uint32_t cclCanMakeExtendedIdentifier(uint32_t identifier);
extern uint32_t cclCanMakeStandardIdentifier(uint32_t identifier);
extern uint32_t cclCanValueOfIdentifier(uint32_t identifier);
extern int32_t cclCanIsExtendedIdentifier(uint32_t identifier);
extern int32_t cclCanIsStandardIdentifier(uint32_t identifier);
extern int32_t cclCanGetChannelNumber(const char *networkName, int32_t *channel);

// ============================================================================
// LIN
// ============================================================================

typedef enum {
    CCL_LIN_ERROR_CHECKSUM,
    CCL_LIN_ERROR_TRANSMISSION,
    CCL_LIN_ERROR_RECEIVE,
    CCL_LIN_ERROR_SYNC,
    CCL_LIN_ERROR_SLAVETIMEOUT
} cclLinErrorType;

struct cclLinFrame {
    int64_t timestampEOF;
    int64_t timestampSOF;
    int64_t timestampEOH;
    int64_t timeSyncBreak;
    int64_t timeSyncDel;
    int32_t channel;
    uint32_t id;
    uint8_t dir;
    uint8_t dlc;
    uint8_t data[8];
};

struct cclLinError {
    int64_t time;
    int32_t channel;
    uint32_t id;
    cclLinErrorType type;
    uint16_t crc;
    uint8_t dlc;
    uint8_t dir;
    uint8_t FSMId;
    uint8_t FSMState;
    uint8_t fullTime;
    uint8_t headerTime;
    uint8_t simulated;
    uint8_t data[8];
    uint8_t offendingByte;
    uint8_t shortError;
    uint8_t stateReason;
    uint8_t timeoutDuringDlcDetection;
    uint8_t followState;
    uint8_t slaveId;
    uint8_t stateId;
};

struct cclLinSlaveResponse {
    int64_t time;
    int32_t channel;
    uint8_t dir;
    uint32_t id;
    uint32_t flags;
    uint32_t dlc;
    uint8_t data[8];
};

struct cclLinPreDispatchMessage {
    int64_t time;
    int32_t channel;
    uint8_t dir;
    uint32_t id;
    uint32_t flags;
    uint32_t dlc;
    uint8_t data[8];
};

struct cclLinSleepModeEvent {
    int64_t time;
    int32_t channel;
    uint8_t external;
    uint8_t isAwake;
    uint8_t reason;
    uint8_t wasAwake;
};

struct cclLinWakeupFrame {
    int64_t time;
    int32_t channel;
    uint8_t external;
    uint8_t signal;
};

#define CCL_LIN_ALLMESSAGES ((uint32_t ) 0xFFFFFFFF)

extern int32_t cclLinSetFrameHandler(int32_t channel,
                                     uint32_t identifier,
                                     void (*function)(struct cclLinFrame *frame));

extern int32_t cclLinSetErrorHandler(int32_t channel,
                                     uint32_t identifier,
                                     void (*function)(struct cclLinError *error));

extern int32_t cclLinUpdateResponseData(int32_t channel,
                                        uint32_t id,
                                        uint8_t dlc,
                                        uint8_t data[8]);

extern int32_t cclLinSendHeader(int32_t channel, uint32_t id);
extern int32_t cclLinStartScheduler(int32_t channel);
extern int32_t cclLinStopScheduler(int32_t channel);
extern int32_t cclLinChangeSchedtable(int32_t channel, uint32_t tableIndex);
extern int32_t cclLinGetChannelNumber(const char *networkName, int32_t *channel);
extern int32_t cclLinGotoSleep(int32_t channel);
extern int32_t cclLinGotoSleepInternal(int32_t channel);
extern int32_t cclLinWakeup(int32_t channel);
extern int32_t cclLinWakeupInternal(int32_t channel);
extern int32_t cclLinSetWakeupCondition(int32_t channel,
                                        uint32_t numWakeupSignals,
                                        uint32_t forgetWupsAfterMS);

extern int32_t cclLinSetWakeupTimings(int32_t channel,
                                      uint32_t ttobrk,
                                      uint32_t wakeupSignalCount,
                                      uint32_t widthinMicSec);

extern int32_t cclLinSetWakeupBehavior(int32_t channel,
                                       uint32_t restartScheduler,
                                       uint32_t wakeupIdentifier,
                                       uint32_t sendShortHeader,
                                       uint32_t wakeupDelimiterLength);

extern int32_t cclLinSetSlaveResponseChangedHandler(int32_t channel,
                                                    uint32_t identifier,
                                                    void (*function)(struct cclLinSlaveResponse *frame));

extern int32_t cclLinSetPreDispatchMessageHandler(int32_t channel,
                                                  uint32_t identifier,
                                                  void (*function)(struct cclLinPreDispatchMessage *frame));

extern int32_t cclLinSetSleepModeEventHandler(int32_t channel,
                                              void (*function)(struct cclLinSleepModeEvent *event));

extern int32_t cclLinSetWakeupFrameHandler(int32_t channel,
                                           void (*function)(struct cclLinWakeupFrame *frame));

// ============================================================================
// Function Bus
// ============================================================================

// types
typedef int64_t cclTime;

#define CCL_TIME_OFFSET_NEVER ((cclTime) -1LL)
#define CCL_TIME_OFFSET_NOW = ((cclTime)  0LL)

// enumerations
typedef enum {
    CCL_IMPL,
    CCL_RAW,
    CCL_PHYS
} cclValueRepresentation;

typedef enum {
    CCL_SERVICE_STATE_UNAVAILABLE,
    CCL_SERVICE_STATE_INITIALIZING,
    CCL_SERVICE_STATE_AVAILABLE
} cclServiceStateProvider;

typedef enum {
    CCL_CONNECTION_STATE_CONSUMER_UNAVAILABLE,
    CCL_CONNECTION_STATE_CONSUMER_CONNECTABLE,
    CCL_CONNECTION_STATE_CONSUMER_AVAILABLE
} cclConnectionStateConsumer;

typedef enum {
    CCL_PROVIDED_CONNECTION_STATE_UNAVAILABLE,
    CCL_PROVIDED_CONNECTION_STATE_CONNECTABLE,
    CCL_PROVIDED_CONNECTION_STATE_CONNECTED
} cclProvidedConnectionState;

typedef enum {
    CCL_SUBSCRIPTION_STATE_UNAVAILABLE,
    CCL_SUBSCRIPTION_STATE_AVAILABLE,
    CCL_SUBSCRIPTION_STATE_SUBSCRIBED
} cclSubscriptionState;

typedef enum {
    CCL_DOMEMBER_SUBSCRIPTION_STATE_UNSUBSCRIBED,
    CCL_DOMEMBER_SUBSCRIPTION_STATE_REQUESTED,
    CCL_DOMEMBER_SUBSCRIPTION_STATE_SUBSCRIBED
} cclDOMemberSubscriptionState;

typedef enum {
    CCL_ANNOUNCEMENT_STATE_UNANNOUNCED,
    CCL_ANNOUNCEMENT_STATE_ANNOUNCED
} cclDOMemberAnnouncementState;

typedef enum {
    CCL_BEHAVIOR_USE_DEFAULT_VALUE,
    CCL_BEHAVIOR_USE_IN_VALUE
} cclParameterBehavior;

typedef enum {
    CCL_ANSWER_MODE_AUTO,
    CCL_ANSWER_MODE_SUSPEND,
    CCL_ANSWER_MODE_DISCARD
} cclAnswerMode;

typedef enum {
    CCL_CALL_STATE_CALLING,
    CCL_CALL_STATE_CALLED,
    CCL_CALL_STATE_RETURNING,
    CCL_CALL_STATE_RETURNED
} cclCallState;

typedef enum {
    CCL_SIDE_CONSUMER,
    CCL_SIDE_PROVIDER
} cclSide;

typedef enum {
    CCL_VALUE_TYPE_INTEGER,
    CCL_VALUE_TYPE_FLOAT,
    CCL_VALUE_TYPE_STRING,
    CCL_VALUE_TYPE_ARRAY,
    CCL_VALUE_TYPE_STRUCT,
    CCL_VALUE_TYPE_UNION,
    CCL_VALUE_TYPE_ENUM
} cclValueType;

typedef enum {
    CCL_VALUE_STATE_OFFLINE,
    CCL_VALUE_STATE_MEASURED
} cclValueState;

// string constants for member paths
#define CCL_MEMBER_SERVICE_STATE      ".ServiceState"
#define CCL_MEMBER_CONNECTION_STATE   ".ConnectionState"
#define CCL_MEMBER_SUBSCRIPTION_STATE ".SubscriptionState"
#define CCL_MEMBER_LATEST_CALL        ".LatestCall"
#define CCL_MEMBER_LATEST_RETURN      ".LatestReturn"
#define CCL_MEMBER_IN_PARAM           ".In"
#define CCL_MEMBER_OUT_PARAM          ".Out"
#define CCL_MEMBER_RESULT             ".Result"
#define CCL_MEMBER_PARAM_DEFAULTS     ".ParamDefaults"
#define CCL_MEMBER_DEFAULT_RESULT     ".DefaultResult"
#define CCL_MEMBER_VALUE              ".Value"
#define CCL_MEMBER_BEHAVIOR           ".Behavior"
#define CCL_MEMBER_AUTO_ANSWER_MODE   ".AutoAnswerMode"
#define CCL_MEMBER_AUTO_ANSWER_TIME   ".AutoAnswerTimeNS"
#define CCL_MEMBER_CALL_TIME          ".CallTime"
#define CCL_MEMBER_RETURN_TIME        ".ReturnTime"
#define CCL_MEMBER_CALL_STATE         ".State"
#define CCL_MEMBER_REQUEST_ID         ".ReqID"
#define CCL_MEMBER_SIDE               ".Side"
#define CCL_MEMBER_CONSUMER_PORT      ".ConsumerPort"
#define CCL_MEMBER_PROVIDER_PORT      ".ProviderPort"
#define CCL_MEMBER_ANNOUNCEMENT_STATE ".AnnouncementState"

// Value Entity access
typedef int32_t cclValueID;
cclValueID cclValueGetID(cclDomain domain, const char *path, const char *member, cclValueRepresentation repr);

int32_t cclValueGetInteger(cclValueID valueID, int64_t *x);
int32_t cclValueSetInteger(cclValueID valueID, int64_t x);
int32_t cclValueGetFloat(cclValueID valueID, double *x);
int32_t cclValueSetFloat(cclValueID valueID, double x);
int32_t cclValueGetString(cclValueID valueID, char *buffer, int32_t bufferSize);
int32_t cclValueSetString(cclValueID valueID, const char *str);
int32_t cclValueGetData(cclValueID valueID, uint8_t *buffer, int32_t *bufferSize);
int32_t cclValueSetData(cclValueID valueID, const uint8_t *data, int32_t size);
int32_t cclValueGetEnum(cclValueID valueID, int32_t *x);
int32_t cclValueSetEnum(cclValueID valueID, int32_t x);
int32_t cclValueGetUnionSelector(cclValueID valueID, int32_t *selector);
int32_t cclValueGetArraySize(cclValueID valueID, int32_t *size);
int32_t cclValueSetArraySize(cclValueID valueID, int32_t size);

int32_t cclValueGetType(cclValueID valueID, cclValueType *valueType);
int32_t cclValueIsValid(cclValueID valueID, bool *isValid);
int32_t cclValueClear(cclValueID valueID);

int32_t cclValueGetUpdateTimeNS(cclValueID valueID, cclTime *time);
int32_t cclValueGetChangeTimeNS(cclValueID valueID, cclTime *time);
int32_t cclValueGetState(cclValueID valueID, cclValueState *state);

typedef void(*cclValueHandler)(cclTime time, cclValueID valueID);
int32_t cclValueSetHandler(cclValueID valueID, bool onUpdate, cclValueHandler handler);

// Network Function Calls
typedef int32_t cclFunctionID;
cclFunctionID cclFunctionGetID(cclDomain domain, const char *path);

typedef int32_t cclCallContextID;
cclCallContextID cclCreateCallContext(cclFunctionID functionID);
cclCallContextID cclCallContextMakePermanent(cclCallContextID ccID);
int32_t cclCallContextRelease(cclCallContextID ccID);

cclValueID cclCallContextValueGetID(cclCallContextID ccID, const char *member, cclValueRepresentation repr);

typedef void(*cclCallHandler)(cclTime time, cclCallContextID ccID);
int32_t cclCallAsync(cclCallContextID ccID, cclCallHandler resultHandler);
int32_t cclCallSync(cclCallContextID ccID, cclCallHandler resultHandler);

int32_t cclFunctionSetHandler(cclFunctionID functionID, cclCallState callState, cclCallHandler handler);
int32_t cclCallContextReturn(cclCallContextID ccID, cclTime timeOffset);
int32_t cclCallContextSetDefaultAnswer(cclCallContextID ccID);
int32_t cclCallContextGetFunctionID(cclCallContextID ccID, cclFunctionID *functionID);
int32_t cclCallContextGetFunctionName(cclCallContextID ccID, char *buffer, int32_t bufferSize);
int32_t cclCallContextGetFunctionPath(cclCallContextID ccID, char *buffer, int32_t bufferSize);
int32_t cclCallContextIsSynchronousCall(cclCallContextID ccID, bool *isSynchronous);

// Service access
typedef int32_t cclServiceID;
typedef int32_t cclConsumerID;
typedef int32_t cclProviderID;
typedef int32_t cclConsumedServiceID;
typedef int32_t cclProvidedServiceID;
typedef int32_t cclAddressID;

#define CCL_CONSUMER_NONE  ((cclConsumerID) -1)
#define CCL_PROVIDER_NONE  ((cclProviderID) -1)

cclServiceID cclServiceGetID(const char *path);

int32_t cclServiceSetSimulator(cclProvidedServiceID providedService, cclCallHandler simulator);

cclConsumerID cclAddConsumerByName(cclServiceID serviceID, const char *name, bool isSimulated);
cclProviderID cclAddProviderByName(cclServiceID serviceID, const char *name, bool isSimulated);
cclConsumerID cclAddConsumerByAddress(cclServiceID serviceID, cclAddressID addressID);
cclProviderID cclAddProviderByAddress(cclServiceID serviceID, cclAddressID addressID);
int32_t cclRemoveConsumer(cclServiceID serviceID, cclConsumerID consumerID);
int32_t cclRemoveProvider(cclServiceID serviceID, cclProviderID providerID);
cclConsumerID cclGetConsumer(cclServiceID serviceID, int32_t bindingID);
cclProviderID cclGetProvider(cclServiceID serviceID, int32_t bindingID);
cclConsumedServiceID cclGetConsumedService(cclServiceID serviceID, int32_t consumerID, int32_t providerID);
cclProvidedServiceID cclGetProvidedService(cclServiceID serviceID, int32_t consumerID, int32_t providerID);

cclConsumerID cclConsumerGetID(const char *path);
cclProviderID cclProviderGetID(const char *path);
int32_t cclProvideService(cclProviderID providerID);
int32_t cclReleaseService(cclProviderID providerID);
int32_t cclIsServiceProvided(cclProviderID providerID, bool *isProvided);
int32_t cclConsumerGetBindingID(cclConsumerID consumerID, int32_t *bindingID);
int32_t cclProviderGetBindingID(cclProviderID providerID, int32_t *bindingID);

cclConsumedServiceID cclConsumedServiceGetID(const char *path);
cclProvidedServiceID cclProvidedServiceGetID(const char *path);
cclValueID
cclConsumedServiceValueGetID(cclConsumedServiceID consumedServiceID, const char *member, cclValueRepresentation repr);
cclValueID
cclProvidedServiceValueGetID(cclProvidedServiceID providedServiceID, const char *member, cclValueRepresentation repr);

int32_t cclRequestService(cclConsumedServiceID consumedServiceID);
int32_t cclReleaseServiceRequest(cclConsumedServiceID consumedServiceID);
int32_t cclIsServiceRequested(cclConsumedServiceID consumedServiceID, bool *isRequested);

int32_t cclDiscoverProviders(cclConsumerID consumerID);
int32_t cclAnnounceProvider(cclProviderID providerID);
int32_t cclUnannounceProvider(cclProviderID providerID);
int32_t cclAnnounceProviderToConsumer(cclServiceID serviceID, cclProviderID providerID, cclConsumerID consumerID);

int32_t cclSetConsumerAddress(cclConsumerID consumerID, cclAddressID addressID, cclProviderID providerID);
int32_t cclSetProviderAddress(cclProviderID providerID, cclAddressID addressID, cclConsumerID consumerID);

typedef void(*cclConnectionEstablishedHandler)();
typedef void(*cclConnectionFailedHandler)(const char *error);

int32_t cclConnectAsyncConsumer(cclConsumedServiceID consumedServiceID, cclConnectionEstablishedHandler success,
                                cclConnectionFailedHandler failure);
int32_t cclConnectAsyncProvider(cclProvidedServiceID providedServiceID, cclConnectionEstablishedHandler success,
                                cclConnectionFailedHandler failure);
int32_t cclDisconnectConsumer(cclConsumedServiceID consumedServiceID);
int32_t cclDisconnectProvider(cclProvidedServiceID providedServiceID);

typedef void(*cclConsumerDiscoveredHandler)(cclAddressID addressID);
typedef void(*cclProviderDiscoveredHandler)(cclAddressID addressID);

int32_t cclSetConsumerDiscoveredHandler(cclProviderID providerID, cclConsumerDiscoveredHandler handler);
int32_t cclSetProviderDiscoveredHandler(cclConsumerID consumerID, cclProviderDiscoveredHandler handler);

typedef void(*cclServiceDiscoveryHandler)(cclConsumerID consumerID);
typedef void(*cclConnectionRequestedHandler)(cclConsumerID consumerID);

int32_t cclSetServiceDiscoveryHandler(cclServiceID serviceID, cclServiceDiscoveryHandler handler);
int32_t cclSetServiceConsumerDiscoveredHandler(cclServiceID serviceID, cclConsumerDiscoveredHandler handler);
int32_t cclSetServiceProviderDiscoveredHandler(cclServiceID serviceID, cclProviderDiscoveredHandler handler);
int32_t cclSetConnectionRequestedHandler(cclProviderID providerID, cclConnectionRequestedHandler handler);

// PDU Signal access
typedef int32_t cclPDUSenderID;
typedef int32_t cclPDUReceiverID;
typedef int32_t cclPDUProviderID;

#define CCL_PDU_SENDER_NONE    ((cclPDUSenderID)   -1)
#define CCL_PDU_RECEIVER_NONE  ((cclPDUReceiverID) -1)
#define CCL_PDU_PROVIDER_NONE  ((cclPDUProviderID) -1)

cclPDUSenderID cclPDUSenderGetID(const char *path);
cclPDUReceiverID cclPDUReceiverGetID(const char *path);
cclPDUProviderID cclPDUProviderGetID(const char *path);

cclValueID cclPDUSenderSignalValueGetID(cclPDUSenderID pduSenderID, const char *signal, const char *member,
                                        cclValueRepresentation repr);
cclValueID cclPDUReceiverSignalValueGetID(cclPDUReceiverID pduReceiverID, const char *signal, const char *member,
                                          cclValueRepresentation repr);
cclValueID cclPDUProviderSignalValueGetID(cclPDUProviderID pduProviderID, const char *signal, const char *member,
                                          cclValueRepresentation repr);

// Service PDU Subscription (binding unspecific)
cclPDUReceiverID cclConsumedPDUGetID(const char *path);
cclPDUSenderID cclProvidedPDUGetID(const char *path);

int32_t cclPDUGetNrOfSubscribedConsumers(cclPDUProviderID pduProviderID, int32_t *nrOfConsumers);
int32_t
cclPDUGetSubscribedConsumers(cclPDUProviderID pduProviderID, cclConsumerID *consumerBuffer, int32_t *bufferSize);

int32_t cclProvidedPDUSetSubscriptionStateIsolated(cclPDUSenderID providedPDUID, bool subscribed);
int32_t cclConsumedPDUSetSubscriptionStateIsolated(cclPDUReceiverID consumedPDUID, bool subscribed);

// Event Subscription (binding unspecific)
typedef int32_t cclConsumedEventID;
typedef int32_t cclProvidedEventID;
typedef int32_t cclEventProviderID;

#define CCL_CONSUMED_EVENT_NONE ((cclConsumedEventID) -1)
#define CCL_PROVIDED_EVENT_NONE ((cclProvidedEventID) -1)
#define CCL_EVENT_PROVIDER_NONE ((cclEventProviderID) -1)

cclConsumedEventID cclConsumedEventGetID(const char *path);
cclProvidedEventID cclProvidedEventGetID(const char *path);
cclEventProviderID cclEventProviderGetID(const char *path);

int32_t cclEventGetNrOfSubscribedConsumers(cclEventProviderID eventProviderID, int32_t *nrOfConsumers);
int32_t
cclEventGetSubscribedConsumers(cclEventProviderID eventProviderID, cclConsumerID *consumerBuffer, int32_t *bufferSize);

int32_t cclConsumedEventSetSubscriptionStateIsolated(cclConsumedEventID consumedEventID, bool subscribed);
int32_t cclProvidedEventSetSubscriptionStateIsolated(cclProvidedEventID providedEventID, bool subscribed);

// Service Field Access
typedef int32_t cclConsumedFieldID;
typedef int32_t cclProvidedFieldID;
typedef int32_t cclFieldProviderID;

#define CCL_CONSUMED_FIELD_NONE ((cclConsumedFieldID) -1)
#define CCL_PROVIDED_FIELD_NONE ((cclProvidedFieldID) -1)
#define CCL_FIELD_PROVIDER_NONE ((cclFieldProviderID) -1)

cclConsumedFieldID cclConsumedFieldGetID(const char *path);
cclProvidedFieldID cclProvidedFieldGetID(const char *path);
cclFieldProviderID cclFieldProviderGetID(const char *path);

int32_t cclFieldGetNrOfSubscribedConsumers(cclFieldProviderID providerID, int32_t *nrOfConsumers);
int32_t
cclFieldGetSubscribedConsumers(cclFieldProviderID providerID, cclConsumerID *consumerBuffer, int32_t *bufferSize);

int32_t cclConsumedFieldSetSubscriptionStateIsolated(cclConsumedFieldID consumedFieldID, bool subscribed);
int32_t cclProvidedFieldSetSubscriptionStateIsolated(cclProvidedFieldID providedFieldID, bool subscribed);

// Abstract binding
cclAddressID cclAbstractCreateAddress(cclServiceID serviceID, const char *endPointName);
int32_t cclAbstractGetBindingID(cclAddressID addressID, int32_t *bindingID);
int32_t cclAbstractGetDisplayName(cclAddressID addressID, char *buffer, int32_t bufferSize);

cclConsumedEventID cclAbstractConsumedEventGetID(const char *path);
cclEventProviderID cclAbstractEventProviderGetID(const char *path);

int32_t cclAbstractRequestEvent(cclConsumedEventID consumedEventID);
int32_t cclAbstractReleaseEvent(cclConsumedEventID consumedEventID);
int32_t cclAbstractIsEventRequested(cclConsumedEventID consumedEventID, bool *isRequested);

int32_t cclAbstractSubscribeEvent(cclConsumedEventID consumedEventID);
int32_t cclAbstractUnsubscribeEvent(cclConsumedEventID consumedEventID);

typedef void(*cclAbstractEventSubscriptionHandler)(cclProvidedEventID providedEventID);
int32_t
cclAbstractSetEventSubscribedHandler(cclEventProviderID providerID, cclAbstractEventSubscriptionHandler handler);
int32_t
cclAbstractSetEventUnsubscribedHandler(cclEventProviderID providerID, cclAbstractEventSubscriptionHandler handler);

cclPDUReceiverID cclAbstractConsumedPDUGetID(const char *path);
cclPDUProviderID cclAbstractPDUProviderGetID(const char *path);

int32_t cclAbstractRequestPDU(cclPDUReceiverID consumedPDUID);
int32_t cclAbstractReleasePDU(cclPDUReceiverID consumedPDUID);
int32_t cclAbstractIsPDURequested(cclPDUReceiverID consumedPDUID, bool *isRequested);

int32_t cclAbstractSubscribePDU(cclPDUReceiverID consumedPDUID);
int32_t cclAbstractUnsubscribePDU(cclPDUReceiverID consumedPDUID);

typedef void(*cclAbstractPDUSubscriptionHandler)(cclPDUSenderID providedPDUID);
int32_t cclAbstractSetPDUSubscribedHandler(cclPDUProviderID pduProviderID, cclAbstractPDUSubscriptionHandler handler);
int32_t cclAbstractSetPDUUnsubscribedHandler(cclPDUProviderID pduProviderID, cclAbstractPDUSubscriptionHandler handler);

cclConsumedFieldID cclAbstractConsumedFieldGetID(const char *path);
cclFieldProviderID cclAbstractFieldProviderGetID(const char *path);

int32_t cclAbstractRequestField(cclConsumedFieldID fieldID);
int32_t cclAbstractReleaseField(cclConsumedFieldID fieldID);
int32_t cclAbstractIsFieldRequested(cclConsumedFieldID fieldID, bool *isRequested);

int32_t cclAbstractSubscribeField(cclConsumedFieldID fieldID);
int32_t cclAbstractUnsubscribeField(cclConsumedFieldID fieldID);

typedef void(*cclAbstractFieldSubscriptionHandler)(cclProvidedFieldID fieldID);
int32_t cclAbstractSetFieldSubscribedHandler(cclFieldProviderID fieldID, cclAbstractFieldSubscriptionHandler handler);
int32_t cclAbstractSetFieldUnsubscribedHandler(cclFieldProviderID fieldID, cclAbstractFieldSubscriptionHandler handler);

// SOME/IP binding
cclAddressID cclSomeIPCreateAddress(const char *udpAddress, int32_t udpPort, const char *tcpAddress, int32_t tcpPort);
int32_t cclSomeIPGetBindingID(cclAddressID addressID, int32_t *bindingID);
int32_t cclSomeIPGetDisplayName(cclAddressID addressID, char *buffer, int32_t bufferSize);

typedef int32_t cclSomeIPConsumedEventGroupID;
typedef int32_t cclSomeIPProvidedEventGroupID;
typedef int32_t cclSomeIPEventGroupProviderID;

cclSomeIPConsumedEventGroupID cclSomeIPConsumedEventGroupGetID(const char *path);
cclSomeIPProvidedEventGroupID cclSomeIPProvidedEventGroupGetID(const char *path);
cclSomeIPEventGroupProviderID cclSomeIPEventGroupProviderGetID(const char *path);

int32_t cclSomeIPRequestEventGroup(cclSomeIPConsumedEventGroupID consumedEventGroupID);
int32_t cclSomeIPReleaseEventGroup(cclSomeIPConsumedEventGroupID consumedEventGroupID);
int32_t cclSomeIPIsEventGroupRequested(cclSomeIPConsumedEventGroupID consumedEventGroupID, bool *isRequested);

int32_t
cclSomeIPConsumedEventGroupGetEvents(cclSomeIPConsumedEventGroupID consumedEventGroupID, cclConsumedEventID *buffer,
                                     int32_t *bufferSize);
int32_t
cclSomeIPProvidedEventGroupGetEvents(cclSomeIPProvidedEventGroupID providedEventGroupID, cclProvidedEventID *buffer,
                                     int32_t *bufferSize);
int32_t
cclSomeIPEventGroupProviderGetEvents(cclSomeIPEventGroupProviderID eventGroupProviderID, cclEventProviderID *buffer,
                                     int32_t *bufferSize);

int32_t cclSomeIPConsumedEventGroupGetPDUs(cclSomeIPConsumedEventGroupID consumedEventGroupID, cclPDUReceiverID *buffer,
                                           int32_t *bufferSize);
int32_t cclSomeIPProvidedEventGroupGetPDUs(cclSomeIPProvidedEventGroupID providedEventGroupID, cclPDUSenderID *buffer,
                                           int32_t *bufferSize);
int32_t cclSomeIPEventGroupProviderGetPDUs(cclSomeIPEventGroupProviderID eventGroupProviderID, cclPDUProviderID *buffer,
                                           int32_t *bufferSize);

int32_t
cclSomeIPConsumedEventGroupGetFields(cclSomeIPConsumedEventGroupID consumedEventGroupID, cclConsumedFieldID *buffer,
                                     int32_t *bufferSize);
int32_t
cclSomeIPProvidedEventGroupGetFields(cclSomeIPProvidedEventGroupID providedEventGroupID, cclProvidedFieldID *buffer,
                                     int32_t *bufferSize);
int32_t
cclSomeIPEventGroupProviderGetFields(cclSomeIPEventGroupProviderID eventGroupProviderID, cclFieldProviderID *buffer,
                                     int32_t *bufferSize);

int32_t cclSomeIPEventGroupGetNrOfSubscribedConsumers(cclSomeIPEventGroupProviderID eventGroupProviderID,
                                                      int32_t *nrOfConsumers);
int32_t cclSomeIPEventGroupGetSubscribedConsumers(cclSomeIPEventGroupProviderID eventGroupProviderID,
                                                  cclConsumerID *consumerBuffer, int32_t *bufferSize);

int32_t cclSomeIPSubscribeEventGroup(cclSomeIPConsumedEventGroupID consumedEventGroupID);
int32_t cclSomeIPUnsubscribeEventGroup(cclSomeIPConsumedEventGroupID consumedEventGroupID);

typedef void(*cclSomeIPEventGroupSubscriptionHandler)(cclSomeIPProvidedEventGroupID providedEventGroupID);
int32_t cclSomeIPSetEventGroupSubscribedHandler(cclSomeIPEventGroupProviderID eventGroupProviderID,
                                                cclSomeIPEventGroupSubscriptionHandler handler);
int32_t cclSomeIPSetEventGroupUnsubscribedHandler(cclSomeIPEventGroupProviderID eventGroupProviderID,
                                                  cclSomeIPEventGroupSubscriptionHandler handler);

int32_t cclSetGpbOSI3Data(uint8_t *data, int32_t dataSize);
int32_t cclSetGpbOSI3Data2(uint8_t *data, int32_t dataSize, int32_t dataType);

// ============================================================================
// Ethernet 
// ============================================================================

extern int32_t cclEthernetOutputPacket(int32_t channel, uint32_t packetSize, const uint8_t data[]);

struct cclEthernetPacket {
    int64_t time;
    int32_t channel;
    uint8_t dir;
    uint32_t packetSize;
    uint8_t *packetData;
};

extern int32_t cclEthernetSetPacketHandler(int32_t channel, void(*function)(struct cclEthernetPacket *packet));

extern int32_t cclEthernetGetChannelNumber(const char *networkName, int32_t *channel);


#if defined __cplusplus
}
#endif
