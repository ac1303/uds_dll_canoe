/* ====================================================================================================================
File Name     CCL/CCL.cpp
Project       CANalyzer/CANoe C Library API (CCL)
Revision      2.0
Date          2021-01-15
Copyright (c) Vector Informatik GmbH. All rights reserved.

Description
The module implements the CANalyzer/CANoe C Library API.
==================================================================================================================== */

#include "CCL.h"

#if defined(_WIN32)

#  include <windows.h>

#elif defined(__linux__)
#  include <dlfcn.h>
#endif


#define _BUILDNODELAYERDLL

#include "VIA.h"
#include "VIA_CAN.h"
#include "VIA_LIN.h"
#include "VIA_SignalAccess.h"
#include "VIA_FunctionBus.h"
#include "VIA_ADAS.h"
#include "IADASDataAccess.h"
#include "VIA_Ethernet.h"

#include <vector>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <limits>
#include <cstdarg>
#include <memory.h>
#include <string.h>
#include <malloc.h>

namespace CANalyzerCANoeCLibrary {
    class VNodeLayerModule;

    class VNodeLayer;

#pragma region global Variables

    // ============================================================================
    // global Variables
    // ============================================================================

#if defined(_WIN32)
    HINSTANCE gDLLInstanceHandle;
#endif

    VNodeLayerModule *gModule = nullptr;
    VIAService *gVIAService = nullptr;
    int32_t gVIAMinorVersion = 0;
    bool gHasBusNames = false;
    VIANamespace *gSysVarRoot = nullptr;
    VNodeLayer *gMasterLayer = nullptr;
    VIASignalAccessApi *gSignalAccessAPI = nullptr;
    VIAFbViaService *gFunctionBusAPI = nullptr;
    VIAADASService *gADASService = nullptr;

    const int32_t cMaxChannel = 32;

    struct CanBusContext {
        CanBusContext() : mBus(nullptr), mNode(nullptr), mLayer(nullptr), mBusName() {}

        VIACan *mBus;
        VIANode *mNode;
        VNodeLayer *mLayer;
        std::string mBusName;
    } gCanBusContext[cMaxChannel + 1];

    struct LinBusContext {
        LinBusContext() : mBus(nullptr), mNode(nullptr), mLayer(nullptr), mBusName() {}

        VIALin *mBus;
        VIANode *mNode;
        VNodeLayer *mLayer;
        std::string mBusName;
    } gLinBusContext[cMaxChannel + 1];

    struct EthernetBusContext {
        EthernetBusContext() : mBus(nullptr), mNode(nullptr), mLayer(nullptr), mBusName() {}

        VIAEthernetBus *mBus;
        VIANode *mNode;
        VNodeLayer *mLayer;
        std::string mBusName;
    } gEthernetBusContext[cMaxChannel + 1];

#pragma endregion

#pragma region State

    // ============================================================================
    // State
    // ============================================================================

    enum State {
        eNotInitialized = 1,
        eLoaded = 2,
        eInitMeasurement = 4,
        eRunMeasurement = 8,
        eStopMeasurement = 16,
        eUnloaded = 32,
    } gState = eNotInitialized;


    inline bool IsInState(int32_t states) {
        return (gState & states) != 0;
    }

#define CCL_STATECHECK(states)  if (!IsInState(states)) { return CCL_WRONGSTATE; }

#pragma endregion

    // ============================================================================
    // Utility functions
    // ============================================================================

    // Secure variant of a string copy function, that always places a termination zero
    // at the destination buffer. If the destination buffer is to small, the destination
    // string will be truncated.
    void UtilStringCopy(char *destination, size_t destinationSize, const char *source) {
        if (destination == nullptr || destinationSize <= 0) {
            return;
        }

        if (source == nullptr) {
            destination[0] = char(0);
            return;
        }

        size_t maximum = destinationSize - 1;
        size_t count = 0;
        for (const char *p = source; (count < maximum) && ((*p) != char(0)); count++, p++) {
            destination[count] = *p;
        }
        destination[count] = char(0);
    }

    // Secure variant of a string copy function, that always places a termination zero
    // at the destination buffer. If the destination buffer is to small, the destination
    // string will be truncated. At most n bytes of the source string are copied.
    void UtilStringCopyN(char *destination, size_t destinationSize, const char *source, size_t n) {
        if (destination == nullptr || destinationSize <= 0) {
            return;
        }

        if (source == nullptr || n <= 0) {
            destination[0] = char(0);
            return;
        }

        size_t maximum = (n < destinationSize - 1) ? n : destinationSize - 1;
        size_t count = 0;
        for (const char *p = source; (count < maximum) && ((*p) != char(0)); count++, p++) {
            destination[count] = *p;
        }
        destination[count] = char(0);
    }

#pragma region FunctionBus API and Utilities

    // ============================================================================
    // Function Bus API
    // ============================================================================

    VIAFbViaService *GetFunctionBusAPI() {
        if (gFunctionBusAPI == nullptr) {
            gVIAService->GetFunctionBusService(&gFunctionBusAPI, VIAFunctionBusServiceMajorVersion,
                                               VIAFunctionBusServiceMinorVersion);
        }
        return gFunctionBusAPI;
    }

    namespace NDetail {
        const int32_t cInvalidCCLObjectID = -1;
        const uint64_t cInvalidPortID = static_cast<uint64_t>(-1LL);

        bool MapTypeLevel(cclValueRepresentation repr, VIAFbTypeLevel &typeLevel) {
            switch (repr) {
                case cclValueRepresentation::CCL_IMPL:
                    typeLevel = VIAFbTypeLevel::eImpl;
                    return true;
                case cclValueRepresentation::CCL_PHYS:
                    typeLevel = VIAFbTypeLevel::ePhys;
                    return true;
                case cclValueRepresentation::CCL_RAW:
                    typeLevel = VIAFbTypeLevel::eRaw;
                    return true;
            }
            return false;
        }

        /// Simple guard class for releasing VIA objects from stack, which requires calling method
        /// Release instead of deleting the pointer.
        template<typename T>
        class VIAObjectGuard {
        public:
            VIAObjectGuard(T *viaObject = nullptr)
                    : mVIAObject(viaObject) {}

            ~VIAObjectGuard() {
                if (mVIAObject != nullptr) mVIAObject->Release();
            }

            VIAObjectGuard(const VIAObjectGuard &) = delete;

            VIAObjectGuard(VIAObjectGuard &&) = delete;

            VIAObjectGuard &operator=(const VIAObjectGuard &) = delete;

            VIAObjectGuard &operator=(VIAObjectGuard &&) = delete;

            // Support releasing ownership of the VIA object, i.e. not destroying it upon guard destruction
            T *Release() {
                T *retval = mVIAObject;
                mVIAObject = nullptr;
                return retval;
            }

            T *mVIAObject;
        };

        /// Helper class for identifying path specifications which start with a reserved keyword.
        /// A simple (ordered) collection of these keywords in turn try to identify the prefix.
        template<typename T>
        class MemberKeyword {
        public:
            MemberKeyword(const char *kw, T vc, const char *pf)
                    : mKeyword(kw), mValueClass(vc), mPrefix(pf) {}

            bool TryReplace(std::string &memberPath, T &valueClass) {
                if (memberPath.compare(0, mKeyword.length(), mKeyword) == 0) {
                    if (mPrefix != nullptr)
                        memberPath.replace(0, mKeyword.length(), mPrefix);
                    else
                        memberPath.erase(0, mKeyword.length());

                    valueClass = mValueClass;
                    return true;
                }
                return false;
            }

        private:
            std::string mKeyword;          // CCL keyword
            T mValueClass;                 // value class associated with the keyword
            const char *mPrefix;           // hard-coded member in value entity struct
        };

        /// Identifies the use of a specific keyword for C API value objects and returns an enumerated
        /// value for either the keyword or general port value use.
        void ExtractValueClass(std::string &remaining, VIAFbValueClass &valueClass) {
            static std::vector<MemberKeyword < VIAFbValueClass>>
            sMemberKeywords;
            if (sMemberKeywords.empty()) {
                sMemberKeywords.push_back(
                        MemberKeyword<VIAFbValueClass>(CCL_MEMBER_SERVICE_STATE + 1, VIAFbValueClass::eServiceState,
                                                       nullptr));
                sMemberKeywords.push_back(MemberKeyword<VIAFbValueClass>(CCL_MEMBER_CONNECTION_STATE + 1,
                                                                         VIAFbValueClass::eConnectionState, nullptr));
                sMemberKeywords.push_back(MemberKeyword<VIAFbValueClass>(CCL_MEMBER_SUBSCRIPTION_STATE + 1,
                                                                         VIAFbValueClass::eSubscriptionState, nullptr));
                sMemberKeywords.push_back(
                        MemberKeyword<VIAFbValueClass>(CCL_MEMBER_LATEST_CALL + 1, VIAFbValueClass::eLatestCall,
                                                       nullptr));
                sMemberKeywords.push_back(MemberKeyword<VIAFbValueClass>(CCL_MEMBER_LATEST_RETURN CCL_MEMBER_RESULT + 1,
                                                                         VIAFbValueClass::eLatestReturn,
                                                                         "ReturnValue")); // keep before LATEST_RETURN
                sMemberKeywords.push_back(
                        MemberKeyword<VIAFbValueClass>(CCL_MEMBER_LATEST_RETURN + 1, VIAFbValueClass::eLatestReturn,
                                                       nullptr));
                sMemberKeywords.push_back(
                        MemberKeyword<VIAFbValueClass>(CCL_MEMBER_PARAM_DEFAULTS + 1, VIAFbValueClass::eParamDefaults,
                                                       nullptr));
                sMemberKeywords.push_back(
                        MemberKeyword<VIAFbValueClass>(CCL_MEMBER_DEFAULT_RESULT + 1, VIAFbValueClass::eParamDefaults,
                                                       "ReturnValue"));
                sMemberKeywords.push_back(
                        MemberKeyword<VIAFbValueClass>(CCL_MEMBER_AUTO_ANSWER_MODE + 1, VIAFbValueClass::eParamDefaults,
                                                       "ServerSimulatorMode"));
                sMemberKeywords.push_back(
                        MemberKeyword<VIAFbValueClass>(CCL_MEMBER_AUTO_ANSWER_TIME + 1, VIAFbValueClass::eParamDefaults,
                                                       "ReturnDelay"));
                sMemberKeywords.push_back(MemberKeyword<VIAFbValueClass>(CCL_MEMBER_ANNOUNCEMENT_STATE + 1,
                                                                         VIAFbValueClass::eAnnouncementState, nullptr));
            }

            for (auto &&entry: sMemberKeywords) {
                if (entry.TryReplace(remaining, valueClass))
                    return;
            }

            // if no keywords match the member path, the value must refer to the port's value entity directly
            valueClass = VIAFbValueClass::ePortValue;
        }

        enum class CallContextValueClass {
            eUnknown,
            eInParam,
            eOutParam,
            eResultValue,
            eCallTime,
            eReturnTime,
            eCallState,
            eRequestID,
            eSide,
            eConsumerPort,
            eProviderPort
        };

        /// Identifies the use of a specific keyword for C API call context objects and returns an
        /// enumerated value (call context value path must always start with a keyword)
        void ExtractValueClass(std::string &remaining, CallContextValueClass &valueClass) {
            static std::vector<MemberKeyword < CallContextValueClass>>
            sMemberKeywords;
            if (sMemberKeywords.empty()) {
                sMemberKeywords.push_back(
                        MemberKeyword<CallContextValueClass>(CCL_MEMBER_IN_PARAM + 1, CallContextValueClass::eInParam,
                                                             nullptr));
                sMemberKeywords.push_back(
                        MemberKeyword<CallContextValueClass>(CCL_MEMBER_OUT_PARAM + 1, CallContextValueClass::eOutParam,
                                                             nullptr));
                sMemberKeywords.push_back(
                        MemberKeyword<CallContextValueClass>(CCL_MEMBER_RESULT + 1, CallContextValueClass::eResultValue,
                                                             "ReturnValue"));
                sMemberKeywords.push_back(
                        MemberKeyword<CallContextValueClass>(CCL_MEMBER_CALL_TIME + 1, CallContextValueClass::eCallTime,
                                                             nullptr));
                sMemberKeywords.push_back(MemberKeyword<CallContextValueClass>(CCL_MEMBER_RETURN_TIME + 1,
                                                                               CallContextValueClass::eReturnTime,
                                                                               nullptr));
                sMemberKeywords.push_back(MemberKeyword<CallContextValueClass>(CCL_MEMBER_CALL_STATE + 1,
                                                                               CallContextValueClass::eCallState,
                                                                               nullptr));
                sMemberKeywords.push_back(MemberKeyword<CallContextValueClass>(CCL_MEMBER_REQUEST_ID + 1,
                                                                               CallContextValueClass::eRequestID,
                                                                               nullptr));
                sMemberKeywords.push_back(
                        MemberKeyword<CallContextValueClass>(CCL_MEMBER_SIDE + 1, CallContextValueClass::eSide,
                                                             nullptr));
                sMemberKeywords.push_back(MemberKeyword<CallContextValueClass>(CCL_MEMBER_CONSUMER_PORT + 1,
                                                                               CallContextValueClass::eConsumerPort,
                                                                               nullptr));
                sMemberKeywords.push_back(MemberKeyword<CallContextValueClass>(CCL_MEMBER_PROVIDER_PORT + 1,
                                                                               CallContextValueClass::eProviderPort,
                                                                               nullptr));
            }

            for (auto &&entry: sMemberKeywords) {
                if (entry.TryReplace(remaining, valueClass))
                    return;
            }

            // call context member path must always start with a keyword
            valueClass = CallContextValueClass::eUnknown;
        }

        /// Special map implementation for tracking C API wrapper objects and their VIA object delegates.
        /// The map organizes reuse in case of "port" objects, which provide valid port server identifiers.
        /// It further listens for dynamic port destruction and destroys corresponding objects implicitly.
        template<typename ValueType>
        class VIAObjectMap : VIAFbPortObserver {
        public:
            VIAObjectMap()
                    : mNextId(0) {}

            ~VIAObjectMap() {
                Clear();
            }

            VIAObjectMap(const VIAObjectMap &) = delete;

            VIAObjectMap(VIAObjectMap &&) = delete;

            VIAObjectMap &operator=(const VIAObjectMap &) = delete;

            VIAObjectMap &operator=(VIAObjectMap &&) = delete;

            // Retrieve a known wrapper object by it's C API identifier
            ValueType *Get(int32_t id) {
                auto it = mIdMap.find(id);
                if (it == mIdMap.end())
                    return nullptr;

                return it->second.pValue;
            }

            // Add a new wrapper object to be destroyed latest at measurement end (ownership is passed to the map)
            int32_t Add(ValueType *value) {
                uint64_t portID = value->GetPortID();
                auto range = mPortIDMap.equal_range(portID);
                for (auto &i = range.first; i != range.second; ++i) {
                    auto it = mIdMap.find(i->second);
                    if ((it != mIdMap.end()) && value->IsEqual(it->second.pValue)) {
                        // got the same object stored already, delete new object and return identifier of original wrapper
                        delete value;
                        it->second.refCount++;
                        return i->second;
                    }
                }

                // find an unused identifier
                while (mIdMap.find(mNextId) != mIdMap.end()) ++mNextId;

                // initial reference count is set to 1
                mIdMap.insert(std::make_pair(mNextId, ValueReference{value}));
                value->SetId(mNextId);

                if (portID != cInvalidPortID) {
                    // it's a port object, register for port removal notifications (will only be effective for dynamic ports)
                    mPortIDMap.insert(std::make_pair(portID, mNextId));
                    value->RegisterPortObserver(this);
                }

                return mNextId++;
            }

            // Remove a known wrapper object by it's C API identifier
            bool Remove(int32_t id) {
                auto it = mIdMap.find(id);
                if (it == mIdMap.end())
                    return false;

                if (--it->second.refCount == 0) {
                    // this was the last reference -> actually remove the map entries
                    if (it->second.pValue != nullptr) {
                        uint64_t portID = it->second.pValue->GetPortID();
                        if (portID != cInvalidPortID) {
                            // explicit removal of wrapper object -> remove only the exact mapped internal entry
                            auto range = mPortIDMap.equal_range(portID);
                            for (auto &i = range.first; i != range.second; ++i) {
                                if (i->second == id) {
                                    mPortIDMap.erase(i);
                                    break;
                                }
                            }
                        }

                        delete it->second.pValue;
                    }
                    mIdMap.erase(it);

                    // reuse the removed identifier
                    if (id < mNextId) mNextId = id;
                }

                return true;
            }

            // Notification about a dynamic port being removed
            VIASTDDECL OnPortRemoval(uint64_t portID) {
                auto range = mPortIDMap.equal_range(portID);

                // remove all mapped wrapper objects (may be multiple in case of SOME/IP event group ports or FEPs)
                for (auto &i = range.first; i != range.second; ++i) {
                    int32_t id = i->second;

                    const auto &it = mIdMap.find(id);
                    if (it != mIdMap.end()) {
                        if (it->second.pValue != nullptr) delete it->second.pValue;
                        it->second.pValue = nullptr;
                    }

                    // reuse the removed identifier
                    if (id < mNextId) mNextId = id;
                }

                mPortIDMap.erase(portID);

                return kVIA_OK;
            }

            // Remove all objects upon measurement stop
            void Clear() {
                for (const auto &entry: mIdMap) {
                    if (entry.second.pValue != nullptr) delete entry.second.pValue;
                }
                mIdMap.clear();
                mPortIDMap.clear();
                mNextId = 0;
            }

        private:
            int32_t mNextId;

            // holds underlying object pointer and reference count
            struct ValueReference {
                ValueReference(ValueType *v) : pValue(v), refCount(1) {}

                ValueType *pValue;
                int32_t refCount;
            };

            std::map<int32_t, ValueReference> mIdMap;

            // Maps port objects from port server identifier to wrapper object identifier.
            // The C API allows clients to create multiple wrapper objects for the same port path,
            // but above Add method checks for identical wrapper objects in case of the same
            // port identifier. Only SOME/IP event groups may still have different wrappers for
            // the same port, if the event group identifier is different.
            std::multimap<uint64_t, int32_t> mPortIDMap;
        };
    }

#pragma endregion

    // ============================================================================
    // VTimer
    // ============================================================================

    class VTimer : public VIAOnTimerSink {
    public:
        VIASTDDECL OnTimer(VIATime nanoseconds);

        VIATimer *mViaTimer;
        int32_t mTimerID;

        void (*mCallbackFunction)(int64_t time, int32_t ID);
    };

    // ============================================================================
    // VSysVar
    // ============================================================================

    class VSysVar : public VIAOnSysVar, public VIAOnSysVarMember {
    public:
        VIASTDDECL OnSysVar(VIATime nanoseconds, VIASystemVariable *ev);

        VIASTDDECL OnSysVarMember(VIATime nanoseconds, VIASystemVariableMember *ev, VIASysVarClientHandle origin);

        static int32_t LookupVariable(int32_t sysVarID, VSysVar *&sysVar);

        static int32_t LookupVariable(int32_t sysVarID, VSysVar *&sysVar,
                                      VIASysVarType t, bool writable);

        int32_t CheckWriteable();

        VIASystemVariable *mViaSysVar;
        VIASystemVariableMember *mViaSysVarMember;
        std::string mVariableName;
        std::string mMemberName;
        int32_t mSysVarID;

        void (*mCallbackFunction)(int64_t time, int32_t sysVarID);
    };

    // ============================================================================
    // VSignal
    // ============================================================================

    class VSignal : public VIAOnSignal {
    public:
        VIASTDDECL OnSignal(VIASignal *signal, void *userData);

        static int32_t LookupSignal(int32_t signalID, VSignal *&signal);

        VIASignal *mViaSignal;
        int32_t mSignalID;

        void (*mCallbackFunction)(int32_t sysVarID);
    };

    // ============================================================================
    // VCanMessageRequest
    // ============================================================================

    class VCanMessageRequest : public VIAOnCanMessage3 {
    public:
        VIASTDDECL OnMessage(VIATime time,
                             VIAChannel channel,
                             uint8 dir,
                             uint32 id,
                             uint32 flags,
                             uint32 frameLength,
                             VIATime startOfFrame,
                             uint32 mBtrCfgArb,
                             uint32 mBtrCfgData,
                             uint32 mTimeOffsetBrsNs,
                             uint32 mTimeOffsetCrcDelNs,
                             uint16 mBitCount,
                             uint32 mCRC,
                             uint8 dataLength,
                             const uint8 *data);

        VIARequestHandle mHandle;
        CanBusContext *mContext;

        void (*mCallbackFunction)(cclCanMessage *message);
    };

    // ============================================================================
    // VLinMessageRequest
    // ============================================================================

    class VLinMessageRequest : public VIAOnLinMessage2 {
    public:
        VIASTDDECL OnMessage(VIATime timeFrameEnd,
                             VIATime timeFrameStart,
                             VIATime timeHeaderEnd,
                             VIATime timeSynchBreak,
                             VIATime timeSynchDel,
                             VIAChannel channel,
                             uint8 dir,
                             uint32 id,
                             uint8 dlc,
                             uint8 simulated,
                             const uint8 data[8]);

        VIARequestHandle mHandle;
        LinBusContext *mContext;

        void (*mCallbackFunction)(cclLinFrame *frame);
    };

    // ============================================================================
    // VLinErrorRequest
    // ============================================================================

    class VLinErrorRequest : public VIAOnLinCheckSumError,
                             public VIAOnLinTransmissionError,
                             public VIAOnLinReceiveError,
                             public VIAOnLinSyncError,
                             public VIAOnLinSlaveTimeout {
    public:
        VIASTDDECL OnCsError(VIATime time,
                             VIAChannel channel,
                             uint16 crc,
                             uint32 id,
                             uint8 dlc,
                             uint8 dir,
                             uint8 FSMId,
                             uint8 FSMState,
                             uint8 FullTime,
                             uint8 HeaderTime,
                             uint8 simulated,
                             const uint8 data[8]);

        VIASTDDECL OnTransmError(VIATime time,
                                 VIAChannel channel,
                                 uint8 FSMId,
                                 uint32 id,
                                 uint8 FSMState,
                                 uint8 dlc,
                                 uint8 FullTime,
                                 uint8 HeaderTime);

        VIASTDDECL OnReceiveError(VIATime time,
                                  VIAChannel channel,
                                  uint8 FSMId,
                                  uint32 id,
                                  uint8 FSMState,
                                  uint8 dlc,
                                  uint8 FullTime,
                                  uint8 HeaderTime,
                                  uint8 offendingByte,
                                  uint8 shortError,
                                  uint8 stateReason,
                                  uint8 timeoutDuringDlcDetection);

        VIASTDDECL OnSyncError(VIATime time,
                               VIAChannel channel);

        VIASTDDECL OnSlaveTimeout(VIATime time,
                                  VIAChannel channel,
                                  uint8 followState,
                                  uint8 slaveId,
                                  uint8 stateId);

        static const int kMaxHandles = 5;
        VIARequestHandle mHandle[kMaxHandles];
        LinBusContext *mContext;

        void (*mCallbackFunction)(cclLinError *error);
    };

    // ============================================================================
    // VLinSlaveResponseChange
    // ============================================================================

    class VLinSlaveResponseChange : public VIAOnLinSlaveResponseChanged {
    public:
        VIASTDDECL OnLinSlaveResponseChanged(
                VIATime time,
                VIAChannel channel,
                uint8 dir,
                uint32 id,
                uint32 flags,
                uint8 dlc,
                const uint8 data[8]);

        VIARequestHandle mHandle;
        LinBusContext *mContext;

        void (*mCallbackFunction)(cclLinSlaveResponse *frame);
    };

    // ============================================================================
    // VLinPreDispatchMessage
    // ============================================================================

    class VLinPreDispatchMessage : public VIAOnLinPreDispatchMessage {
    public:
        VIASTDDECL OnLinPreDispatchMessage(
                VIATime time,
                VIAChannel channel,
                uint8 dir,
                uint32 id,
                uint32 flags,
                uint8 dlc,
                uint8 data[8]);

        VIARequestHandle mHandle;
        LinBusContext *mContext;

        void (*mCallbackFunction)(cclLinPreDispatchMessage *frame);
    };

    // ============================================================================
    // VLinSleepModeEvent
    // ============================================================================

    class VLinSleepModeEvent : public VIAOnLinSleepModeEvent {
    public:
        VIASTDDECL OnSleepModeEvent(
                VIATime time,
                VIAChannel channel,
                uint8 external,
                uint8 isAwake,
                uint8 reason,
                uint8 wasAwake);

        VIARequestHandle mHandle;
        LinBusContext *mContext;

        void (*mCallbackFunction)(cclLinSleepModeEvent *event);
    };

    // ============================================================================
    // VLinWakeupFrame
    // ============================================================================

    class VLinWakeupFrame : public VIAOnLinWakeupFrame {
    public:
        VIASTDDECL OnWakeupFrame(
                VIATime time,
                VIAChannel channel,
                uint8 external,
                uint8 signal);

        VIARequestHandle mHandle;
        LinBusContext *mContext;

        void (*mCallbackFunction)(cclLinWakeupFrame *frame);
    };

    // ============================================================================
    // VEthernetPacketRequest
    // ============================================================================

    class VEthernetPacketRequest : public VIAOnEthernetPacket {
    public:
        VIASTDDECL OnEthernetPacket(VIATime time, VIAChannel channel, uint8 dir, uint32 packetSize,
                                    const uint8 *packetData);

        VIARequestHandle mHandle;
        EthernetBusContext *mContext;

        void (*mCallbackFunction)(cclEthernetPacket *packet);
    };

    // ============================================================================
    // VPortBase
    // ============================================================================

    template<typename PortType>
    class VPortBase {
    public:
        VPortBase(PortType *port, bool release = true)
                : mPort(port), mRelease(release), mPortID(NDetail::cInvalidPortID), mPortObserverHandle(nullptr) {
            if (port != nullptr) port->GetPortID(&mPortID);
        }

        virtual ~VPortBase() {
            if (mPort != nullptr) {
                if (mPortObserverHandle != nullptr) mPort->UnregisterPortObserver(mPortObserverHandle);
                if (mRelease) mPort->Release();
            }
        }

        virtual uint64_t GetPortID() const {
            return mPortID;
        }

        // override in subclasses if objects are not uniquely identified by their port ID
        virtual bool IsEqual(const VPortBase<PortType> *other) const {
            return (mPortID == other->mPortID);
        }

        virtual void RegisterPortObserver(VIAFbPortObserver *observer) {
            if (mPort != nullptr) mPort->RegisterPortObserver(observer, &mPortObserverHandle);
        }

        PortType *mPort;

    protected:
        bool mRelease;
        uint64_t mPortID;
        VIAFbCallbackHandle mPortObserverHandle;
    };

    // ============================================================================
    // IValue
    // ============================================================================

    // Common value interface for storing regular value entities, call context values
    // and PDU signal values in a single VIAObjectMap, which provides a common ID
    // space for these objects.
    struct IValue {
        virtual ~IValue() {}

        virtual bool IsEqual(const IValue *other) const = 0;

        virtual void SetId(cclValueID id) = 0;

        virtual uint64_t GetPortID() const = 0;

        virtual void RegisterPortObserver(VIAFbPortObserver *observer) = 0;

        virtual int32_t GetInteger(int64_t *x) const = 0;

        virtual int32_t SetInteger(int64_t x) = 0;

        virtual int32_t GetFloat(double *x) const = 0;

        virtual int32_t SetFloat(double x) = 0;

        virtual int32_t GetString(char *buffer, int32_t bufferSize) const = 0;

        virtual int32_t SetString(const char *str) = 0;

        virtual int32_t GetData(uint8_t *buffer, int32_t *bufferSize) const = 0;

        virtual int32_t SetData(const uint8_t *data, int32_t size) = 0;

        virtual int32_t GetEnum(int32_t *x) const = 0;

        virtual int32_t SetEnum(int32_t x) = 0;

        virtual int32_t GetUnionSelector(int32_t *selector) const = 0;

        virtual int32_t GetArraySize(int32_t *size) const = 0;

        virtual int32_t SetArraySize(int32_t size) = 0;

        virtual int32_t GetValueType(cclValueType *valueType) const = 0;

        virtual int32_t IsValid(bool *isValid) const = 0;

        virtual int32_t ClearValue() = 0;

        virtual int32_t GetUpdateTimeNS(cclTime *time) const = 0;

        virtual int32_t GetChangeTimeNS(cclTime *time) const = 0;

        virtual int32_t GetState(cclValueState *state) const = 0;

        virtual int32_t SetHandler(bool onUpdate, cclValueHandler handler) = 0;

        template<typename T>
        static int32_t LookupValue(cclValueID valueID, T *&value);
    };

    // ============================================================================
    // VPortValueEntity
    // ============================================================================

    class VValueBase : public IValue, public VIAFbValueObserver {
    public:
        virtual ~VValueBase();

        VIASTDDECL OnValueUpdated(VIATime inTime, VIAFbStatus *inStatus);

        virtual void SetId(cclValueID id) { mValueID = id; }

        virtual bool IsEqual(const IValue *other) const;

        // IValue interface
        virtual int32_t GetInteger(int64_t *x) const;

        virtual int32_t SetInteger(int64_t x);

        virtual int32_t GetFloat(double *x) const;

        virtual int32_t SetFloat(double x);

        virtual int32_t GetString(char *buffer, int32_t bufferSize) const;

        virtual int32_t SetString(const char *str);

        virtual int32_t GetData(uint8_t *buffer, int32_t *bufferSize) const;

        virtual int32_t SetData(const uint8_t *data, int32_t size);

        virtual int32_t GetEnum(int32_t *x) const;

        virtual int32_t SetEnum(int32_t x);

        virtual int32_t GetUnionSelector(int32_t *selector) const;

        virtual int32_t GetArraySize(int32_t *size) const;

        virtual int32_t SetArraySize(int32_t size);

        virtual int32_t GetValueType(cclValueType *valueType) const;

        virtual int32_t IsValid(bool *isValid) const;

        virtual int32_t ClearValue();

        virtual int32_t GetUpdateTimeNS(cclTime *time) const;

        virtual int32_t GetChangeTimeNS(cclTime *time) const;

        virtual int32_t GetState(cclValueState *state) const;

        virtual int32_t SetHandler(bool onUpdate, cclValueHandler handler);

        cclValueID mValueID;

    protected:
        VValueBase();

        int32_t Init(VIAFbValue *implValue, const std::string &memberPath);

        virtual VIAResult
        AccessValue(VIAFbTypeLevel typeLevel, VIAFbTypeMemberHandle memberHandle, VIAFbValue **value) const = 0;

        virtual bool IsMemberValid() const = 0;

        virtual VIAResult SetValueToVE(VIAFbValue *pValue) = 0;

        VIAFbTypeLevel mTypeLevel;
        VIAFbTypeTag mMemberType;
        VIAFbStatus *mMemberStatus;
        VIAFbTypeMemberHandle mMemberHandle;
        cclValueHandler mCallbackFunction;
        VIAFbCallbackHandle mCallbackHandle;
    };

    class VPortValueBase : public VPortBase<VIAFbValuePort>, public VValueBase {
    public:
        virtual ~VPortValueBase();


        virtual bool IsEqual(const IValue *other) const;

        virtual uint64_t GetPortID() const { return VPortBase<VIAFbValuePort>::GetPortID(); }

        virtual void
        RegisterPortObserver(VIAFbPortObserver *observer) { VPortBase<VIAFbValuePort>::RegisterPortObserver(observer); }

    protected:
        VPortValueBase(VIAFbValuePort *port);
    };


    class VPortValueEntity : public VPortValueBase {
    public:
        virtual ~VPortValueEntity();

        static int32_t CreateValueEntity(const char *portPath, const char *memberPath, cclValueRepresentation repr);

        static int32_t CreateValueEntity(VIAFbValuePort *port, const char *memberPath, cclValueRepresentation repr);

        // IValue interface overrides

        virtual bool IsEqual(const IValue *other) const;

    protected:
        virtual VIAResult
        AccessValue(VIAFbTypeLevel typeLevel, VIAFbTypeMemberHandle memberHandle, VIAFbValue **value) const;

        virtual bool IsMemberValid() const;

        virtual int32_t SetValueToVE(VIAFbValue *pValue);

    private:
        VPortValueEntity(VIAFbValuePort *port);

        int32_t Init(const char *memberPath, cclValueRepresentation repr);

        int32_t InitPduSignal(const char *signalName, const char *memberPath, cclValueRepresentation repr);

        int32_t InitImplementation(const char *signalName, const char *memberPath, cclValueRepresentation repr);

        VIAFbValueClass mValueClass;
        std::string mSignalName;
    };

    class VDOMemberBase {
    public:
        virtual ~VDOMemberBase();

        uint64_t GetPortID() const;

        void RegisterPortObserver(VIAFbPortObserver *observer) {}

        static int32_t GetDOMemberByPath(const char *inPath, VIAFbDOMember **outDOMember);

    protected:
        VDOMemberBase(VIAFbDOMember *pDOMember)
                : mMember(pDOMember) {
        }

        VIAFbDOMember *mMember;
    };

    class VDOValueEntity : public VValueBase, public VDOMemberBase {
    public:
        virtual ~VDOValueEntity();

        static int32_t CreateValueEntity(const char *portPath, const char *memberPath, cclValueRepresentation repr);

        virtual bool IsEqual(const IValue *other) const;

        virtual uint64_t GetPortID() const { return VDOMemberBase::GetPortID(); }

        virtual void RegisterPortObserver(VIAFbPortObserver *observer) {
            return VDOMemberBase::RegisterPortObserver(observer);
        }

    protected:
        virtual VIAResult
        AccessValue(VIAFbTypeLevel typeLevel, VIAFbTypeMemberHandle memberHandle, VIAFbValue **value) const;

        virtual bool IsMemberValid() const;

        virtual int32_t SetValueToVE(VIAFbValue *pValue);

    private:
        explicit VDOValueEntity(VIAFbDOMember *pDOMember);

        int32_t Init(const char *memberPath, cclValueRepresentation repr);

        VIAFbValueClass mValueClass;
    };

    class VCallContextValue : public VPortValueBase {
    public:
        virtual ~VCallContextValue();

        static int32_t
        CreateCallContextValue(cclCallContextID ctxtID, const char *memberPath, cclValueRepresentation repr);

        // IValue interface overrides
        virtual int32_t GetInteger(int64_t *x) const;

        virtual int32_t SetInteger(int64_t x);

        virtual int32_t GetFloat(double *x) const;

        virtual int32_t SetFloat(double x);

        virtual int32_t GetString(char *buffer, int32_t bufferSize) const;

        virtual int32_t SetString(const char *str);

        virtual int32_t GetData(uint8_t *buffer, int32_t *bufferSize) const;

        virtual int32_t SetData(const uint8_t *data, int32_t size);

        virtual int32_t GetEnum(int32_t *x) const;

        virtual int32_t SetEnum(int32_t x);

        virtual int32_t GetUnionSelector(int32_t *selector) const;

        virtual int32_t GetArraySize(int32_t *size) const;

        virtual int32_t SetArraySize(int32_t size);

        virtual int32_t GetValueType(cclValueType *valueType) const;

        virtual int32_t IsValid(bool *isValid) const;

        virtual int32_t ClearValue();

    protected:
        virtual VIAResult
        AccessValue(VIAFbTypeLevel typeLevel, VIAFbTypeMemberHandle memberHandle, VIAFbValue **value) const;

        virtual bool IsMemberValid() const;

        virtual int32_t SetValueToVE(VIAFbValue *pValue);

    private:
        VCallContextValue();

        int32_t Init(cclCallContextID ctxtID, const char *memberPath, cclValueRepresentation repr);

        VIAFbValue *GetParamsValue(VIAFbCallContext *ctxt, bool *isValidMember) const;

        int32_t SetParamsValue(VIAFbCallContext *ctxt, VIAFbValue *value) const;

        VIAFbCallContext *GetCallContext() const;

        cclCallContextID mCallContextID;
        NDetail::CallContextValueClass mValueClass;

        static cclCallState MapCallState(VIAFbFunctionCallState callState);

        static VIAFbFunctionCallState MapCallState(cclCallState callState);
    };

    template<typename PortType>
    class VPDUSignalValue : public VPortBase<PortType>, public IValue, public VIAFbValueObserver {
    public:
        virtual ~VPDUSignalValue();

        static int32_t
        CreateSignalValue(PortType *port, const char *signalName, const char *member, cclValueRepresentation repr);

        VIASTDDECL OnValueUpdated(VIATime inTime, VIAFbStatus *inStatus);

        // IValue interface
        virtual int32_t GetInteger(int64_t *x) const;

        virtual int32_t SetInteger(int64_t x);

        virtual int32_t GetFloat(double *x) const;

        virtual int32_t SetFloat(double x);

        virtual int32_t GetString(char *buffer, int32_t bufferSize) const;

        virtual int32_t SetString(const char *str);

        virtual int32_t GetData(uint8_t *buffer, int32_t *bufferSize) const;

        virtual int32_t SetData(const uint8_t *data, int32_t size);

        virtual int32_t GetEnum(int32_t *x) const;

        virtual int32_t SetEnum(int32_t x);

        virtual int32_t GetUnionSelector(int32_t *selector) const;

        virtual int32_t GetArraySize(int32_t *size) const;

        virtual int32_t SetArraySize(int32_t size);

        virtual int32_t GetValueType(cclValueType *valueType) const;

        virtual int32_t IsValid(bool *isValid) const;

        virtual int32_t ClearValue();

        virtual int32_t GetUpdateTimeNS(cclTime *time) const;

        virtual int32_t GetChangeTimeNS(cclTime *time) const;

        virtual int32_t GetState(cclValueState *state) const;

        virtual int32_t SetHandler(bool onUpdate, cclValueHandler handler);

        virtual void SetId(cclValueID id) { mValueID = id; }

        virtual bool IsEqual(const IValue *other) const;

        virtual uint64_t GetPortID() const { return VPortBase<PortType>::GetPortID(); }

        virtual void RegisterPortObserver(VIAFbPortObserver *observer) {
            VPortBase<PortType>::RegisterPortObserver(observer);
        }

        cclValueID mValueID;

    private:
        VPDUSignalValue(PortType *port);

        int32_t Init(const char *signalName, const char *member, cclValueRepresentation repr);

        VIAFbValue *GetMemberValue() const;

        bool IsMemberValid() const;

        std::string mSignalName;
        VIAFbTypeLevel mTypeLevel;
        VIAFbTypeTag mMemberType;
        VIAFbStatus *mMemberStatus;
        VIAFbTypeMemberHandle mMemberHandle;

        cclValueHandler mCallbackFunction;
        VIAFbCallbackHandle mCallbackHandle;
        VIAFbCallbackHandle mPortObserverHandle;
    };

    // ============================================================================
    // V[Client|Server]Function
    // ============================================================================

    class VCallContext;

    // need a common interface for storing any side function ports in NDetail::VIAObjectMap
    struct IFunctionPort {
        virtual ~IFunctionPort() {}

        // stores the objects own identifier when added to a VIAObjectMap
        virtual void SetId(cclFunctionID id) = 0;

        // retrieves the native port server's unique identifier
        virtual uint64_t GetPortID() const = 0;

        // registers with the port server for dynamic port removal
        virtual void RegisterPortObserver(VIAFbPortObserver *observer) = 0;

        // compares this port to given other port for reusing wrapper objects
        virtual bool IsEqual(const IFunctionPort *other) const = 0;

        // register a callback for a specific call state of the function
        virtual int32_t SetHandler(int32_t callbackIndex, VIAFbFunctionCallState state, cclCallHandler handler) = 0;

        // returns whether the function port is consumer side or provider side
        virtual cclSide GetSide() = 0;

        // returns the objects C API identifier
        virtual cclFunctionID GetFunctionID() = 0;

        // returns the function port's path
        virtual std::string GetPath() = 0;

        // starts a call if on client side
        virtual cclCallContextID BeginCall() = 0;

        // completes the call if on client side
        virtual int32_t InvokeCall(VCallContext *callContext) = 0;

        virtual int32_t InvokeSynchronousCall(VCallContext *callContext) = 0;
    };

    template<typename T, typename IF, typename Subclass>
    class VFunctionBase : public T, public IFunctionPort, public IF {
    protected:
        template<typename U>
        VFunctionBase(U *port, cclSide side, const char *portPath)
                : T(port), mFunctionID(-1), mSide(side), mPath(portPath), mFinalizingHandle(nullptr) {
            static_cast<Subclass *>(this)->RegisterCallStateObserver(VIAFbFunctionCallState::eFinalizing,
                                                                     &mFinalizingHandle);
        }

    public:

        virtual ~VFunctionBase() {
            for (int32_t i = 0; i < CALLSTATE_COUNT; ++i) {
                if (mCallbacks[i].mCallbackHandle != nullptr) {
                    static_cast<Subclass *>(this)->UnregisterCallStateObserver(mCallbacks[i].mCallbackHandle);

                    mCallbacks[i].mCallbackHandle = nullptr;
                    mCallbacks[i].mCallbackFunction = nullptr;
                }
            }

            if (mFinalizingHandle != nullptr) {
                static_cast<Subclass *>(this)->UnregisterCallStateObserver(mFinalizingHandle);

                mFinalizingHandle = nullptr;
            }
        }

        virtual void SetId(cclFunctionID id) override { mFunctionID = id; }

        virtual uint64_t GetPortID() const override { return T::GetPortID(); }

        virtual void RegisterPortObserver(VIAFbPortObserver *observer) override { T::RegisterPortObserver(observer); }

        virtual bool IsEqual(const IFunctionPort *other) const override {
            return other->GetPortID() == this->GetPortID();
        }

        virtual cclSide GetSide() override { return mSide; }

        virtual cclFunctionID GetFunctionID() override { return mFunctionID; }

        virtual std::string GetPath() override { return mPath; }

        cclFunctionID mFunctionID;
        cclSide mSide;
        std::string mPath;

        // handling callbacks
        virtual int32_t
        SetHandler(int32_t callbackIndex, VIAFbFunctionCallState state, cclCallHandler handler) override {
            auto &callback = mCallbacks[callbackIndex];
            if (callback.mCallbackHandle != nullptr) {
                static_cast<Subclass *>(this)->UnregisterCallStateObserver(callback.mCallbackHandle);
                callback.mCallbackHandle = nullptr;
                callback.mCallbackFunction = nullptr;
            }

            callback.mCallbackFunction = handler;

            if (callback.mCallbackFunction != nullptr) {
                if (static_cast<Subclass *>(this)->RegisterCallStateObserver(state, &callback.mCallbackHandle) !=
                    kVIA_OK) {
                    callback.mCallbackFunction = nullptr;
                    callback.mCallbackHandle = nullptr;
                    return CCL_INTERNALERROR;
                }
            }

            return CCL_SUCCESS;
        }

        struct Callback {
            Callback() : mCallbackFunction(nullptr), mCallbackHandle(nullptr) {}

            cclCallHandler mCallbackFunction;
            VIAFbCallbackHandle mCallbackHandle;
        };

        static const int32_t CALLSTATE_COUNT = 4;
        Callback mCallbacks[CALLSTATE_COUNT]; // one callback per CallState (excluding Finalizing)
        VIAFbCallbackHandle mFinalizingHandle;
    };

    namespace VFunctionUtils {
        static int32_t LookupFunction(cclFunctionID functionID, IFunctionPort *&function);

        static int32_t MapCallState(cclCallState callState, VIAFbFunctionCallState &mapped);

        static int32_t MapCallState(VIAFbFunctionCallState callState, cclCallState &mapped);

        // call context management
        static cclCallContextID CreateCallContext(VIAFbCallContext *ctxt, cclFunctionID funcID, bool release);

        static bool DestroyCallContext(cclCallContextID ctxtID);
    }

    template<typename T, typename IF, typename Subclass>
    class VClientFunctionBase : public VFunctionBase<T, IF, Subclass> {
    protected:
        using VFunctionBase<T, IF, Subclass>::VFunctionBase;

        // tracking created (non-temporary) call contexts for reuse in callbacks
        std::map<int64_t, cclCallContextID> mRequestIDMap;

        VIAResult HandleCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                         VIAFbFunctionCallState inCallState);

        cclCallContextID FindCallContext(VIAFbCallContext *ctxt);

        cclCallContextID CreateCallContext(VIAFbCallContext *ctxt);

        bool DestroyCallContext(VIAFbCallContext *ctxt);
    };

    class VClientFunction
            : public VClientFunctionBase<VPortBase<VIAFbFunctionClientPort>, VIAFbFunctionClientObserver, VClientFunction> {
    public:
        VClientFunction(VIAFbFunctionClientPort *port, const char *portPath);

        virtual ~VClientFunction();

        static int32_t CreateClientFunction(const char *portPath);

        VIASTDDECL OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                      VIAFbFunctionCallState inCallState, const VIAFbFunctionClientPort *inClientPort);

        virtual cclCallContextID BeginCall();

        virtual int32_t InvokeCall(VCallContext *callContext);

        virtual int32_t InvokeSynchronousCall(VCallContext *callContext) { return CCL_NO_SYNCHRONOUS_CALL; }

        VIAResult RegisterCallStateObserver(VIAFbFunctionCallState callState, VIAFbCallbackHandle *handle) {
            return mPort != nullptr ? mPort->RegisterObserver(this, callState, handle) : kVIA_Failed;
        }

        void UnregisterCallStateObserver(VIAFbCallbackHandle handle) {
            if (mPort != nullptr) mPort->UnregisterObserver(handle);
        }
    };

    template<typename T, typename IF, typename Subclass>
    class VServerFunctionBase : public VFunctionBase<T, IF, Subclass> {
    protected:
        using VFunctionBase<T, IF, Subclass>::VFunctionBase;

        VIAResult HandleCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                         VIAFbFunctionCallState inCallState);

        virtual cclCallContextID BeginCall() { return CCL_INTERNALERROR; }

        virtual int32_t InvokeCall(VCallContext *callContext) { return CCL_INTERNALERROR; }

        virtual int32_t InvokeSynchronousCall(VCallContext *callContext) { return CCL_INTERNALERROR; }
    };

    class VServerFunction
            : public VServerFunctionBase<VPortBase<VIAFbFunctionServerPort>, VIAFbFunctionServerObserver, VServerFunction> {
    public:
        VServerFunction(VIAFbFunctionServerPort *port, const char *portPath);

        virtual ~VServerFunction();

        static int32_t CreateServerFunction(const char *portPath);

        VIASTDDECL OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                      VIAFbFunctionCallState inCallState, const VIAFbFunctionServerPort *inServerPort);

        VIAResult RegisterCallStateObserver(VIAFbFunctionCallState callState, VIAFbCallbackHandle *handle) {
            return mPort != nullptr ? mPort->RegisterObserver(this, callState, handle) : kVIA_Failed;
        }

        void UnregisterCallStateObserver(VIAFbCallbackHandle handle) {
            if (mPort != nullptr) mPort->UnregisterObserver(handle);
        }
    };

    class VMethodBase : public VDOMemberBase {
    public:
        using VDOMemberBase::VDOMemberBase;

        virtual ~VMethodBase();

    protected:
        VIAResult DoRegisterCallStateObserver(VIAFbDOMethodObserver *pObserver, VIAFbFunctionCallState callState,
                                              VIAFbCallbackHandle *handle);

        void DoUnregisterCallStateObserver(VIAFbCallbackHandle handle);

    private:
        VIAFbDOMethod *mMethod = nullptr;
    };

    class VConsumedMethod : public VClientFunctionBase<VMethodBase, VIAFbDOMethodObserver, VConsumedMethod> {
    public:
        VConsumedMethod(VIAFbDOMember *doMethod, const char *doMemberPath);

        virtual ~VConsumedMethod();

        VIASTDDECL OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                      VIAFbFunctionCallState inCallState, const VIAFbDOMethod *inMethod);

        virtual cclCallContextID BeginCall();

        virtual int32_t InvokeCall(VCallContext *callContext);

        virtual int32_t InvokeSynchronousCall(VCallContext *callContext);

        VIAResult RegisterCallStateObserver(VIAFbFunctionCallState callState, VIAFbCallbackHandle *handle) {
            return DoRegisterCallStateObserver(this, callState, handle);
        }

        void UnregisterCallStateObserver(VIAFbCallbackHandle handle) {
            DoUnregisterCallStateObserver(handle);
        }
    };

    class VProvidedMethod : public VServerFunctionBase<VMethodBase, VIAFbDOMethodObserver, VProvidedMethod> {
    public:
        VProvidedMethod(VIAFbDOMember *doMethod, const char *doMemberPath);

        virtual ~VProvidedMethod();

        VIASTDDECL OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                      VIAFbFunctionCallState inCallState, const VIAFbDOMethod *inMethod);

        VIAResult RegisterCallStateObserver(VIAFbFunctionCallState callState, VIAFbCallbackHandle *handle) {
            return DoRegisterCallStateObserver(this, callState, handle);
        }

        void UnregisterCallStateObserver(VIAFbCallbackHandle handle) {
            DoUnregisterCallStateObserver(handle);
        }
    };

    namespace DOMethods {
        cclFunctionID CreateDOMethod(const char *doMemberPath);
    }

    class VCallContext : public VIAFbFunctionClientObserver, public VIAFbDOMethodObserver {
    public:
        VCallContext(cclFunctionID funcID, VIAFbCallContext *ctxt, bool release);

        virtual ~VCallContext();

        VIASTDDECL OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                      VIAFbFunctionCallState inCallState, const VIAFbFunctionClientPort *inClientPort);

        VIASTDDECL OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                      VIAFbFunctionCallState inCallState, const VIAFbDOMethod *inMethod);

        void SetId(cclCallContextID id) { mContextID = id; }

        uint64_t GetPortID() const { return NDetail::cInvalidPortID; }

        void RegisterPortObserver(VIAFbPortObserver *observer) {}

        bool IsEqual(const VCallContext *other) const {
            return (mRequestID == other->mRequestID) && (mSide == other->mSide);
        }

        bool IsSynchronousCall() const;

        cclFunctionID mFunctionID;
        cclCallContextID mContextID;
        VIAFbCallContext *mContext;
        int64_t mRequestID;
        VIAFbFunctionCallSide mSide;
        bool mRelease;

        std::vector<cclValueID> mContextValues;

        static const int64 kIsSynchronousCallFlag = 0x800000000LL;

        cclCallHandler mResultCallbackFunction;
    };

    // ============================================================================
    // VService
    // ============================================================================

    class VService : public VIAFbServiceDiscoveryObserver {
    public:
        virtual ~VService();

        int32_t SetServiceDiscoveryHandler(cclServiceDiscoveryHandler handler);

        int32_t SetConsumerDiscoveredHandler(cclConsumerDiscoveredHandler handler);

        int32_t SetProviderDiscoveredHandler(cclProviderDiscoveredHandler handler);

        static int32_t CreateService(const char *path);

        static int32_t LookupService(cclServiceID serviceID, VService *&service);

        static void Cleanup();

        VIASTDDECL OnServiceDiscovery(VIAFbServiceCO *inService, VIAFbServiceConsumer *inConsumer);

        VIASTDDECL OnConsumerDiscovered(VIAFbServiceCO *inService, VIAFbAddressHandle *inAddress);

        VIASTDDECL OnProviderDiscovered(VIAFbServiceCO *inService, VIAFbAddressHandle *inAddress);

        cclServiceID mServiceID;
        VIAFbServiceCO *mService;

    private:
        VService();

        int32_t UpdateRegistration();

        cclServiceDiscoveryHandler mServiceDiscoveryHandler;
        cclConsumerDiscoveredHandler mConsumerDiscoveredHandler;
        cclProviderDiscoveredHandler mProviderDiscoveredHandler;
        VIAFbCallbackHandle mCallbackHandle;
    };

    // ============================================================================
    // V[Consumer|Provider]
    // ============================================================================

    class VConsumer : public VPortBase<VIAFbServiceConsumer>, public VIAFbServiceConsumerObserver {
    public:
        virtual ~VConsumer();

        int32_t SetProviderDiscoveredHandler(cclProviderDiscoveredHandler handler);

        static int32_t CreateConsumer(const char *path);

        static int32_t CreateConsumer(VIAFbServiceConsumer *consumer, bool release);

        static int32_t LookupConsumer(cclConsumerID consumerID, VConsumer *&consumer);

        static int32_t RemoveConsumer(cclConsumerID consumerID);

        VIASTDDECL OnProviderDiscovered(VIAFbServiceConsumer *inConsumer, VIAFbAddressHandle *inAddress);

        void SetId(cclConsumerID consumerID) { mConsumerID = consumerID; }

        cclConsumerID mConsumerID;

    private:
        VConsumer(VIAFbServiceConsumer *consumer, bool release);

        cclProviderDiscoveredHandler mProviderDiscoveredHandler;
        VIAFbCallbackHandle mCallbackHandle;
    };

    class VProvider : public VPortBase<VIAFbServiceProvider>, public VIAFbServiceProviderObserver {
    public:
        virtual ~VProvider();

        int32_t SetConnectionRequestedHandler(cclConnectionRequestedHandler handler);

        int32_t SetConsumerDiscoveredHandler(cclConsumerDiscoveredHandler handler);

        static int32_t CreateProvider(const char *path);

        static int32_t CreateProvider(VIAFbServiceProvider *provider, bool release);

        static int32_t LookupProvider(cclProviderID providerID, VProvider *&provider);

        static int32_t RemoveProvider(cclProviderID providerID);

        VIASTDDECL OnConsumerDiscovered(VIAFbServiceProvider *inProvider, VIAFbAddressHandle *inAddress);

        VIASTDDECL OnConnectionRequested(VIAFbServiceProvider *inProvider, VIAFbServiceConsumer *inConsumer);

        void SetId(cclProviderID providerID) { mProviderID = providerID; }

        cclProviderID mProviderID;

    private:
        VProvider(VIAFbServiceProvider *provider, bool release);

        int32_t UpdateRegistration();

        cclConnectionRequestedHandler mConnectionRequestedHandler;
        cclConsumerDiscoveredHandler mConsumerDiscoveredHandler;
        VIAFbCallbackHandle mCallbackHandle;
    };

    // ============================================================================
    // V[Consumed|Provided]Service
    // ============================================================================

    class VConsumedService : public VPortBase<VIAFbConsumedService> {
    public:
        virtual ~VConsumedService();

        int32_t
        ConnectAsync(cclConnectionEstablishedHandler successCallback, cclConnectionFailedHandler failureCallback);

        static int32_t CreateConsumedService(const char *path);

        static int32_t CreateConsumedService(VIAFbConsumedService *consumedService);

        static int32_t
        LookupConsumedService(cclConsumedServiceID consumedServiceID, VConsumedService *&consumedService);

        void SetId(cclConsumedServiceID consumedServiceID) { mConsumedServiceID = consumedServiceID; }

        cclConsumedServiceID mConsumedServiceID;

    private:
        VConsumedService(VIAFbConsumedService *consumedService);

        class ConnectionHandler : public VIAFbConsumedServiceConnectionHandler {
        public:
            VIASTDDECL OnConnectionEstablished(VIAFbConsumedService *inPort);

            VIASTDDECL OnConnectionFailed(VIAFbConsumedService *inPort, const char *inError);

            cclConnectionEstablishedHandler mSuccessCallback;
            cclConnectionFailedHandler mFailureCallback;

            VConsumedService *mParent;
        };

        std::vector<ConnectionHandler *> mConnectionHandlerPool;
        std::set<ConnectionHandler *> mActiveConnectionHandlers;
    };

    class VProvidedService : public VPortBase<VIAFbProvidedService>, public VIAFbFunctionServerObserver {
    public:
        virtual ~VProvidedService();

        int32_t SetSimulator(cclCallHandler simulator);

        int32_t
        ConnectAsync(cclConnectionEstablishedHandler successCallback, cclConnectionFailedHandler failureCallback);

        VIASTDDECL OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                      VIAFbFunctionCallState inCallState, const VIAFbFunctionServerPort *inServerPort);

        static int32_t CreateProvidedService(const char *path);

        static int32_t CreateProvidedService(VIAFbProvidedService *providedService);

        static int32_t
        LookupProvidedService(cclProvidedServiceID providedServiceID, VProvidedService *&providedService);

        void SetId(cclProvidedServiceID providedServiceID) { mProvidedServiceID = providedServiceID; }

        cclProvidedServiceID mProvidedServiceID;

        cclCallHandler mSimulatorCallback;
        VIAFbCallbackHandle mSimulatorHandle;

        std::map<std::string, cclFunctionID> mFunctionMap;

    private:
        VProvidedService(VIAFbProvidedService *providedService);

        cclFunctionID FindFunction(VIAFbCallContext *callContext);

        class ConnectionHandler : public VIAFbProvidedServiceConnectionHandler {
        public:
            VIASTDDECL OnConnectionEstablished(VIAFbProvidedService *inPort);

            VIASTDDECL OnConnectionFailed(VIAFbProvidedService *inPort, const char *inError);

            cclConnectionEstablishedHandler mSuccessCallback;
            cclConnectionFailedHandler mFailureCallback;

            VProvidedService *mParent;
        };

        std::vector<ConnectionHandler *> mConnectionHandlerPool;
        std::set<ConnectionHandler *> mActiveConnectionHandlers;
    };

    // ============================================================================
    // VAddress
    // ============================================================================

    class VAddress {
    public:
        ~VAddress();

        static cclAddressID CreateAddress(VIAFbAddressHandle *address, bool release);

        static int32_t LookupAddress(cclAddressID addressID, VAddress *&address);

        static int32_t RemoveAddress(cclAddressID addressID);

        uint64_t GetPortID() const { return NDetail::cInvalidPortID; }

        void RegisterPortObserver(VIAFbPortObserver *observer) {}

        bool IsEqual(const VAddress *other) const { return mAddress == other->mAddress; }

        void SetId(cclAddressID id) { mAddressID = id; }

        cclAddressID mAddressID;
        VIAFbAddressHandle *mAddress;
        bool mRelease;

    private:
        VAddress(VIAFbAddressHandle *address, bool release);
    };

    // ============================================================================
    // VPDU[Sender|Receiver]
    // ============================================================================

    class VPDUSender : public VPortBase<VIAFbPDUSenderPort> {
    public:
        virtual ~VPDUSender();

        static int32_t CreatePDUSender(const char *path);

        static int32_t LookupPDUSender(cclPDUSenderID pduSenderID, VPDUSender *&pduSender);

        void SetId(cclPDUSenderID pduSenderID) { mPDUSenderID = pduSenderID; }

        cclPDUSenderID mPDUSenderID;

    protected:
        VPDUSender(VIAFbPDUSenderPort *pduSender, bool release);
    };

    class VPDUReceiver : public VPortBase<VIAFbPDUReceiverPort> {
    public:
        virtual ~VPDUReceiver();

        static int32_t CreatePDUReceiver(const char *path);

        static int32_t LookupPDUReceiver(cclPDUReceiverID pduReceiverID, VPDUReceiver *&pduReceiver);

        void SetId(cclPDUReceiverID pduReceiverID) { mPDUReceiverID = pduReceiverID; }

        cclPDUReceiverID mPDUReceiverID;

    protected:
        VPDUReceiver(VIAFbPDUReceiverPort *pduReceiver);
    };

    // ============================================================================
    // V[Abstract][Consumed|Provided]ServicePDU
    // ============================================================================

    class VConsumedServicePDU : public VPDUReceiver {
    public:
        virtual ~VConsumedServicePDU();

        static int32_t CreateConsumedPDU(const char *path);

        static int32_t CreateConsumedPDU(VIAFbConsumedServicePDU *consumedPDU);

        static int32_t LookupConsumedPDU(cclPDUReceiverID consumedPDUID, VConsumedServicePDU *&consumedPDU);

        int32_t SetSubscriptionStateIsolated(bool subscribed);

    protected:
        VConsumedServicePDU(VIAFbConsumedServicePDU *consumedPDU);

        VIAFbConsumedServicePDU *mConsumedPDU;
    };

    class VAbstractConsumedServicePDU : public VConsumedServicePDU {
    public:
        virtual ~VAbstractConsumedServicePDU();

        static int32_t CreateConsumedPDU(const char *path);

        static int32_t LookupConsumedPDU(cclPDUReceiverID consumedPDUID, VAbstractConsumedServicePDU *&consumedPDU);

        VIAFbAbstractConsumedServicePDU *mAbstractConsumedPDU;

    private:
        VAbstractConsumedServicePDU(VIAFbAbstractConsumedServicePDU *abstractConsumedPDU);
    };

    class VProvidedServicePDU : public VPDUSender {
    public:
        ~VProvidedServicePDU();

        static int32_t CreateProvidedPDU(const char *path);

        static int32_t CreateProvidedPDU(VIAFbProvidedServicePDU *providedPDU, bool release);

        static int32_t LookupProvidedPDU(cclPDUSenderID providedPDUID, VProvidedServicePDU *&providedPDU);

        static int32_t RemoveProvidedPDU(cclPDUSenderID providedPDUID);

        int32_t SetSubscriptionStateIsolated(bool subscribed);

    private:
        VProvidedServicePDU(VIAFbProvidedServicePDU *providedPDU, bool release);

        VIAFbProvidedServicePDU *mProvidedPDU;
    };

    // ============================================================================
    // V[Abstract]ServicePDUProvider
    // ============================================================================

    class VServicePDUProvider : public VPortBase<VIAFbServicePDUProvider> {
    public:
        virtual ~VServicePDUProvider();

        static int32_t CreatePDUProvider(const char *path);

        static int32_t CreatePDUProvider(VIAFbServicePDUProvider *pduProvider);

        static int32_t LookupPDUProvider(cclPDUProviderID pduProviderID, VServicePDUProvider *&pduProvider);

        int32_t GetNrOfSubscribedConsumers(int32_t *nrOfConsumers);

        int32_t GetSubscribedConsumers(cclConsumerID *consumerBuffer, int32_t *bufferSize);

        void SetId(cclPDUProviderID pduProviderID) { mPDUProviderID = pduProviderID; }

        cclPDUProviderID mPDUProviderID;

    protected:
        VServicePDUProvider(VIAFbServicePDUProvider *pduProvider);
    };

    class VAbstractServicePDUProvider : public VServicePDUProvider, public VIAFbAbstractPDUSubscriptionObserver {
    public:
        virtual ~VAbstractServicePDUProvider();

        static int32_t CreatePDUProvider(const char *path);

        static int32_t LookupPDUProvider(cclPDUProviderID pduProviderID, VAbstractServicePDUProvider *&pduProvider);

        int32_t SetPDUSubscribedHandler(cclAbstractPDUSubscriptionHandler handler);

        int32_t SetPDUUnsubscribedHandler(cclAbstractPDUSubscriptionHandler handler);

        VIASTDDECL OnPDUSubscribed(VIAFbProvidedServicePDU *inPDUPort);

        VIASTDDECL OnPDUUnsubscribed(VIAFbProvidedServicePDU *inPDUPort);

        VIAFbAbstractServicePDUProvider *mAbstractPDUProvider;

    private:
        VAbstractServicePDUProvider(VIAFbAbstractServicePDUProvider *abstractServicePDUProvider);

        int32_t UpdateRegistration();

        cclAbstractPDUSubscriptionHandler mPDUSubscribedHandler;
        cclAbstractPDUSubscriptionHandler mPDUUnsubscribedHandler;
        VIAFbCallbackHandle mCallbackHandle;
    };

    // ============================================================================
    // V[Abstract][Consumed|Provided]Event
    // ============================================================================

    class VConsumedEvent : public VPortBase<VIAFbConsumedEvent> {
    public:
        virtual ~VConsumedEvent();

        static int32_t CreateConsumedEvent(const char *path);

        static int32_t CreateConsumedEvent(VIAFbConsumedEvent *consumedEvent);

        static int32_t LookupConsumedEvent(cclConsumedEventID consumedEventID, VConsumedEvent *&consumedEvent);

        int32_t SetSubscriptionStateIsolated(bool subscribed);

        void SetId(cclConsumedEventID consumedEventID) { mConsumedEventID = consumedEventID; }

        cclConsumedEventID mConsumedEventID;

    protected:
        VConsumedEvent(VIAFbConsumedEvent *consumedEvent);
    };

    class VAbstractConsumedEvent : public VConsumedEvent {
    public:
        virtual ~VAbstractConsumedEvent();

        static int32_t CreateConsumedEvent(const char *path);

        static int32_t LookupConsumedEvent(cclConsumedEventID consumedEventID, VAbstractConsumedEvent *&consumedEvent);

        VIAFbAbstractConsumedEvent *mAbstractConsumedEvent;

    private:
        VAbstractConsumedEvent(VIAFbAbstractConsumedEvent *abstractConsumedEvent);
    };

    class VProvidedEvent : public VPortBase<VIAFbProvidedEvent> {
    public:
        ~VProvidedEvent();

        static int32_t CreateProvidedEvent(const char *path);

        static int32_t CreateProvidedEvent(VIAFbProvidedEvent *providedEvent, bool release);

        static int32_t LookupProvidedEvent(cclProvidedEventID providedEventID, VProvidedEvent *&providedEvent);

        static int32_t RemoveProvidedEvent(cclProvidedEventID providedEventID);

        int32_t SetSubscriptionStateIsolated(bool subscribed);

        void SetId(cclProvidedEventID providedEventID) { mProvidedEventID = providedEventID; }

        cclProvidedEventID mProvidedEventID;

    private:
        VProvidedEvent(VIAFbProvidedEvent *providedEvent, bool release);
    };

    // ============================================================================
    // V[Abstract]EventProvider
    // ============================================================================

    class VEventProvider : public VPortBase<VIAFbEventProvider> {
    public:
        virtual ~VEventProvider();

        static int32_t CreateEventProvider(const char *path);

        static int32_t CreateEventProvider(VIAFbEventProvider *eventProvider);

        static int32_t LookupEventProvider(cclEventProviderID eventProviderID, VEventProvider *&eventProvider);

        int32_t GetNrOfSubscribedConsumers(int32_t *nrOfConsumers);

        int32_t GetSubscribedConsumers(cclConsumerID *consumerBuffer, int32_t *bufferSize);

        void SetId(cclEventProviderID eventProviderID) { mEventProviderID = eventProviderID; }

        cclEventProviderID mEventProviderID;

    protected:
        VEventProvider(VIAFbEventProvider *eventProvider);
    };

    class VAbstractEventProvider : public VEventProvider, public VIAFbAbstractEventSubscriptionObserver {
    public:
        virtual ~VAbstractEventProvider();

        static int32_t CreateEventProvider(const char *path);

        static int32_t LookupEventProvider(cclEventProviderID eventProviderID, VAbstractEventProvider *&eventProvider);

        int32_t SetEventSubscribedHandler(cclAbstractEventSubscriptionHandler handler);

        int32_t SetEventUnsubscribedHandler(cclAbstractEventSubscriptionHandler handler);

        VIASTDDECL OnEventSubscribed(VIAFbProvidedEvent *inEventPort);

        VIASTDDECL OnEventUnsubscribed(VIAFbProvidedEvent *inEventPort);

        VIAFbAbstractEventProvider *mAbstractEventProvider;

    private:
        VAbstractEventProvider(VIAFbAbstractEventProvider *abstractEventProvider);

        int32_t UpdateRegistration();

        cclAbstractEventSubscriptionHandler mEventSubscribedHandler;
        cclAbstractEventSubscriptionHandler mEventUnsubscribedHandler;
        VIAFbCallbackHandle mCallbackHandle;
    };

    // ============================================================================
    // V[Abstract][Consumed|Provided]Field
    // ============================================================================

    class VConsumedField : public VPortBase<VIAFbConsumedField> {
    public:
        virtual ~VConsumedField();

        static int32_t CreateConsumedField(const char *path);

        static int32_t CreateConsumedField(VIAFbConsumedField *consumedField);

        static int32_t LookupConsumedField(cclConsumedFieldID consumedFieldID, VConsumedField *&consumedField);

        int32_t SetSubscriptionStateIsolated(bool subscribed);

        void SetId(cclConsumedFieldID consumedFieldID) { mConsumedFieldID = consumedFieldID; }

        cclConsumedFieldID mConsumedFieldID;

    protected:
        VConsumedField(VIAFbConsumedField *consumedField);
    };

    class VAbstractConsumedField : public VConsumedField {
    public:
        virtual ~VAbstractConsumedField();

        static int32_t CreateConsumedField(const char *path);

        static int32_t LookupConsumedField(cclConsumedFieldID consumedFieldID, VAbstractConsumedField *&consumedField);

        VIAFbAbstractConsumedField *mAbstractConsumedField;

    private:
        VAbstractConsumedField(VIAFbAbstractConsumedField *abstractConsumedField);
    };

    class VProvidedField : public VPortBase<VIAFbProvidedField> {
    public:
        ~VProvidedField();

        static int32_t CreateProvidedField(const char *path);

        static int32_t CreateProvidedField(VIAFbProvidedField *providedField, bool release);

        static int32_t LookupProvidedField(cclProvidedFieldID providedFieldID, VProvidedField *&providedField);

        static int32_t RemoveProvidedField(cclProvidedFieldID providedFieldID);

        int32_t SetSubscriptionStateIsolated(bool subscribed);

        void SetId(cclProvidedFieldID providedFieldID) { mProvidedFieldID = providedFieldID; }

        cclProvidedFieldID mProvidedFieldID;

    private:
        VProvidedField(VIAFbProvidedField *providedField, bool release);
    };

    // ============================================================================
    // V[Abstract]FieldProvider
    // ============================================================================

    class VFieldProvider : public VPortBase<VIAFbFieldProvider> {
    public:
        virtual ~VFieldProvider();

        static int32_t CreateFieldProvider(const char *path);

        static int32_t CreateFieldProvider(VIAFbFieldProvider *fieldProvider);

        static int32_t LookupFieldProvider(cclFieldProviderID fieldProviderID, VFieldProvider *&fieldProvider);

        int32_t GetNrOfSubscribedConsumers(int32_t *nrOfConsumers);

        int32_t GetSubscribedConsumers(cclConsumerID *consumerBuffer, int32_t *bufferSize);

        void SetId(cclFieldProviderID fieldProviderID) { mFieldProviderID = fieldProviderID; }

        cclFieldProviderID mFieldProviderID;

    protected:
        VFieldProvider(VIAFbFieldProvider *fieldProvider);
    };

    class VAbstractFieldProvider : public VFieldProvider, public VIAFbAbstractFieldSubscriptionObserver {
    public:
        virtual ~VAbstractFieldProvider();

        static int32_t CreateFieldProvider(const char *path);

        static int32_t LookupFieldProvider(cclFieldProviderID fieldProviderID, VAbstractFieldProvider *&fieldProvider);

        int32_t SetFieldSubscribedHandler(cclAbstractFieldSubscriptionHandler handler);

        int32_t SetFieldUnsubscribedHandler(cclAbstractFieldSubscriptionHandler handler);

        VIASTDDECL OnFieldSubscribed(VIAFbProvidedField *inFieldPort);

        VIASTDDECL OnFieldUnsubscribed(VIAFbProvidedField *inFieldPort);

        VIAFbAbstractFieldProvider *mAbstractFieldProvider;

    private:
        VAbstractFieldProvider(VIAFbAbstractFieldProvider *abstractFieldProvider);

        int32_t UpdateRegistration();

        cclAbstractFieldSubscriptionHandler mFieldSubscribedHandler;
        cclAbstractFieldSubscriptionHandler mFieldUnsubscribedHandler;
        VIAFbCallbackHandle mCallbackHandle;
    };

    // ============================================================================
    // V[Consumed|Provided]EventGroup
    // ============================================================================

    class VConsumedEventGroup : public VPortBase<VIAFbSomeIPConsumedEventGroup> {
    public:
        ~VConsumedEventGroup();

        static int32_t CreateConsumedEventGroup(const char *path);

        static int32_t LookupConsumedEventGroup(cclSomeIPConsumedEventGroupID consumedEventGroupID,
                                                VConsumedEventGroup *&consumedEventGroup);

        int32_t GetEvents(cclConsumedEventID *eventBuffer, int32_t *bufferSize);

        int32_t GetPDUs(cclPDUReceiverID *pduBuffer, int32_t *bufferSize);

        int32_t GetFields(cclConsumedFieldID *fieldBuffer, int32_t *bufferSize);

        void SetId(cclSomeIPConsumedEventGroupID consumedEventGroupID) { mConsumedEventGroupID = consumedEventGroupID; }

        virtual bool IsEqual(const VPortBase<VIAFbSomeIPConsumedEventGroup> *other) const;

        cclSomeIPConsumedEventGroupID mConsumedEventGroupID;
        uint32_t mEventGroupID;

    private:
        VConsumedEventGroup(VIAFbSomeIPConsumedEventGroup *consumedEventGroup);

        VIAFbCallbackHandle mPortObserverHandle;
    };

    class VProvidedEventGroup : public VPortBase<VIAFbSomeIPProvidedEventGroup> {
    public:
        ~VProvidedEventGroup();

        static int32_t CreateProvidedEventGroup(const char *path);

        static int32_t CreateProvidedEventGroup(VIAFbSomeIPProvidedEventGroup *providedEventGroup);

        static int32_t LookupProvidedEventGroup(cclSomeIPProvidedEventGroupID providedEventGroupID,
                                                VProvidedEventGroup *&providedEventGroup);

        static int32_t RemoveProvidedEventGroup(cclSomeIPProvidedEventGroupID providedEventGroupID);

        int32_t GetEvents(cclProvidedEventID *eventBuffer, int32_t *bufferSize);

        int32_t GetPDUs(cclPDUSenderID *pduBuffer, int32_t *bufferSize);

        int32_t GetFields(cclProvidedFieldID *fieldBuffer, int32_t *bufferSize);

        void SetId(cclSomeIPProvidedEventGroupID providedEventGroupID) { mProvidedEventGroupID = providedEventGroupID; }

        virtual bool IsEqual(const VPortBase<VIAFbSomeIPProvidedEventGroup> *other) const;

        cclSomeIPProvidedEventGroupID mProvidedEventGroupID;
        uint32_t mEventGroupID;

    private:
        VProvidedEventGroup(VIAFbSomeIPProvidedEventGroup *providedEventGroup, bool release);
    };

    // ============================================================================
    // VEventGroupProvider
    // ============================================================================

    class VEventGroupProvider
            : public VPortBase<VIAFbSomeIPEventGroupProvider>, public VIAFbSomeIPEventGroupSubscriptionObserver {
    public:
        virtual ~VEventGroupProvider();

        static int32_t CreateEventGroupProvider(const char *path);

        static int32_t LookupEventGroupProvider(cclSomeIPEventGroupProviderID eventGroupProviderID,
                                                VEventGroupProvider *&eventGroupProvider);

        int32_t GetNrOfSubscribedConsumers(int32_t *nrOfConsumers);

        int32_t GetSubscribedConsumers(cclConsumerID *consumerBuffer, int32_t *bufferSize);

        int32_t SetEventGroupSubscribedHandler(cclSomeIPEventGroupSubscriptionHandler handler);

        int32_t SetEventGroupUnsubscribedHandler(cclSomeIPEventGroupSubscriptionHandler handler);

        int32_t GetEvents(cclEventProviderID *eventBuffer, int32_t *bufferSize);

        int32_t GetPDUs(cclPDUProviderID *pduBuffer, int32_t *bufferSize);

        int32_t GetFields(cclFieldProviderID *fieldBuffer, int32_t *bufferSize);

        VIASTDDECL OnEventGroupSubscribed(VIAFbSomeIPProvidedEventGroup *inEventPort);

        VIASTDDECL OnEventGroupUnsubscribed(VIAFbSomeIPProvidedEventGroup *inEventPort);

        void SetId(cclSomeIPEventGroupProviderID eventGroupProviderID) { mEventGroupProviderID = eventGroupProviderID; }

        virtual bool IsEqual(const VPortBase<VIAFbSomeIPEventGroupProvider> *other) const;

        cclSomeIPEventGroupProviderID mEventGroupProviderID;
        uint32_t mEventGroupID;

    private:
        VEventGroupProvider(VIAFbSomeIPEventGroupProvider *eventGroupProvider);

        int32_t UpdateRegistration();

        cclSomeIPEventGroupSubscriptionHandler mEventGroupSubscribedHandler;
        cclSomeIPEventGroupSubscriptionHandler mEventGroupUnsubscribedHandler;
        VIAFbCallbackHandle mCallbackHandle;
    };

    // ============================================================================
    // VNodeLayerModule
    // ============================================================================

    class VNodeLayerModule
            : public VIAModuleApi {
    public:
        VNodeLayerModule();

        VIASTDDECL Init();

        VIASTDDECL GetVersion(char *buffer, int32 bufferLength);

        VIASTDDECL GetModuleParameters(char *pathBuff, int32 pathBuffLength,
                                       char *versionBuff, int32 versionBuffLength);

        VIASTDDECL CreateObject(VIANodeLayerApi **object,
                                VIANode *node,
                                int32 argc, char *argv[]);

        VIASTDDECL ReleaseObject(VIANodeLayerApi *object);

        VIASTDDECL GetNodeInfo(const char *nodename,
                               char *shortNameBuf, int32 shortBufLength,
                               char *longNameBuf, int32 longBufLength);

        VIASTDDECL InitMeasurement();

        VIASTDDECL EndMeasurement();

        VIASTDDECL GetNodeInfoEx(VIDBNodeDefinition *nodeDefinition,
                                 char *shortNameBuf, int32 shortBufLength,
                                 char *longNameBuf, int32 longBufLength);

        VIASTDDECL DoInformOfChange(VIDBNodeDefinition *nodeDefinition,
                                    const uint32 changeFlags,
                                    const char *simBusName,
                                    const VIABusInterfaceType busType,
                                    const VIAChannel channel,
                                    const char *oldName,
                                    const char *newName,
                                    const int32 bValue);

        void (*mMeasurementPreStartHandler)();

        void (*mMeasurementStartHandler)();

        void (*mMeasurementStopHandler)();

        void (*mDllUnloadHandler)();

        // Objects which cannot be deleted during measurement are stored in vector: their identifier
        // is identical to the index in the vector. Dynamic objects are stored in a VIAObjectMap helper
        // class, which assigns dynamic IDs and manages object removal.
        std::vector<VTimer *> mTimer;
        std::vector<VSysVar *> mSysVar;
        std::vector<VSignal *> mSignal;
        std::vector<VCanMessageRequest *> mCanMessageRequests;
        std::vector<VLinMessageRequest *> mLinMessageRequests;
        std::vector<VLinErrorRequest *> mLinErrorRequests;
        std::vector<VLinSlaveResponseChange *> mLinSlaveResponseChange;
        std::vector<VLinPreDispatchMessage *> mLinPreDispatchMessage;
        std::vector<VLinSleepModeEvent *> mLinSleepModeEvent;
        std::vector<VLinWakeupFrame *> mLinWakeupFrame;
        std::vector<VEthernetPacketRequest *> mEthernetPacketRequests;
        std::vector<VService *> mServices;
        NDetail::VIAObjectMap<IValue> mValues;
        NDetail::VIAObjectMap<IFunctionPort> mFunctions;
        NDetail::VIAObjectMap<VCallContext> mCallContexts;
        NDetail::VIAObjectMap<VConsumer> mConsumers;
        NDetail::VIAObjectMap<VProvider> mProviders;
        NDetail::VIAObjectMap<VConsumedService> mConsumedServices;
        NDetail::VIAObjectMap<VProvidedService> mProvidedServices;
        NDetail::VIAObjectMap<VAddress> mAddresses;
        NDetail::VIAObjectMap<VPDUSender> mPDUSender;
        NDetail::VIAObjectMap<VPDUReceiver> mPDUReceiver;
        NDetail::VIAObjectMap<VServicePDUProvider> mPDUProviders;
        NDetail::VIAObjectMap<VConsumedEvent> mConsumedEvents;
        NDetail::VIAObjectMap<VProvidedEvent> mProvidedEvents;
        NDetail::VIAObjectMap<VEventProvider> mEventProviders;
        NDetail::VIAObjectMap<VConsumedField> mConsumedFields;
        NDetail::VIAObjectMap<VProvidedField> mProvidedFields;
        NDetail::VIAObjectMap<VFieldProvider> mFieldProviders;
        NDetail::VIAObjectMap<VConsumedEventGroup> mConsumedEventGroups;
        NDetail::VIAObjectMap<VProvidedEventGroup> mProvidedEventGroups;
        NDetail::VIAObjectMap<VEventGroupProvider> mEventGroupProviders;
    };

    // ============================================================================
    // VNodeLayer
    // ============================================================================

    class VNodeLayer
            : public VIANodeLayerApi {
    public:
        VNodeLayer(VIANode *node, uint32 busType, VIAChannel channel, VIABus *bus);

        virtual ~VNodeLayer();

        VIASTDDECL GetNode(VIANode **node);

        VIASTDDECL InitMeasurement();

        VIASTDDECL StartMeasurement();

        VIASTDDECL StopMeasurement();

        VIASTDDECL EndMeasurement();

        VIASTDDECL InitMeasurementComplete();

        VIASTDDECL PreInitMeasurement();

        VIASTDDECL PreStopMeasurement();

    public:

        VIANode *mNode;
        uint32 mBusType;
        VIAChannel mChannel;
        VIABus *mBus;
    };

#pragma region VTimer Implementation

    // ============================================================================
    // Implementation of class VTimer
    // ============================================================================

    VIASTDDEF VTimer::OnTimer(VIATime nanoseconds) {
        if (mCallbackFunction != nullptr) {
            mCallbackFunction(nanoseconds, mTimerID);
        }
        return kVIA_OK;
    }

#pragma endregion

#pragma region VSysVar Implementation

    // ============================================================================
    // Implementation of class VSysVar
    // ============================================================================

    VIASTDDEF VSysVar::OnSysVar(VIATime nanoseconds, VIASystemVariable *ev) {
        if (mCallbackFunction != nullptr) {
            mCallbackFunction(nanoseconds, mSysVarID);
        }
        return kVIA_OK;
    }


    VIASTDDEF VSysVar::OnSysVarMember(VIATime nanoseconds, VIASystemVariableMember *ev, VIASysVarClientHandle origin) {
        if (mCallbackFunction != nullptr) {
            mCallbackFunction(nanoseconds, mSysVarID);
        }
        return kVIA_OK;
    }


    int32_t VSysVar::LookupVariable(int32_t sysVarID, VSysVar *&sysVar) {
        if (gModule == nullptr) {
            return CCL_INTERNALERROR;
        }

        if (sysVarID < 0 || sysVarID >= static_cast<int32_t>(gModule->mSysVar.size())) {
            return CCL_INVALIDSYSVARID;
        }

        sysVar = gModule->mSysVar[sysVarID];
        if (sysVar == nullptr || sysVar->mSysVarID != sysVarID) {
            return CCL_INVALIDSYSVARID;
        }

        return CCL_SUCCESS;
    }


    int32_t VSysVar::LookupVariable(int32_t sysVarID, VSysVar *&sysVar, VIASysVarType t, bool writable) {
        int32_t result = LookupVariable(sysVarID, sysVar);
        if (result != CCL_SUCCESS) {
            return result;
        }

        // check type of system variable
        VIASysVarType varType;
        VIAResult rc;
        if (sysVar->mViaSysVarMember != nullptr) {
            rc = sysVar->mViaSysVarMember->GetType(&varType);
        } else {
            rc = sysVar->mViaSysVar->GetType(&varType);
        }
        if (rc != kVIA_OK) {
            if (rc == kVIA_ObjectInvalid) {
                return CCL_SYSVARNOTDEFINED;
            } else {
                return CCL_INTERNALERROR;
            }
        }
        if (!((t == varType) ||
              (t == kVIA_SVData && sysVar->mViaSysVarMember != nullptr && varType == kVIA_SVGenericArray) ||
              (t == kVIA_SVData && varType == kVIA_SVStruct) ||
              (t == kVIA_SVIntegerArray && sysVar->mViaSysVarMember != nullptr && varType == kVIA_SVGenericArray))) {
            return CCL_WRONGTYPE;
        }

        // check, that variable is not read only
        if (writable) {
            int32_t result = sysVar->CheckWriteable();
            if (result != CCL_SUCCESS) {
                return result;
            }
        }

        return CCL_SUCCESS;
    }


    int32_t VSysVar::CheckWriteable() {
        // check, that variable is not read only
        VIAResult rc;
        int32 readOnly;
        rc = mViaSysVar->IsReadOnly(&readOnly);
        if (rc != kVIA_OK) {
            if (rc == kVIA_ObjectInvalid) {
                return CCL_SYSVARNOTDEFINED;
            } else {
                return CCL_INTERNALERROR;
            }
        }
        if (readOnly) {
            return CCL_SYSVARISREADONLY;
        }

        return CCL_SUCCESS;
    }

#pragma endregion

#pragma region VSignal Implementation

    // ============================================================================
    // Implementation of class VSignal
    // ============================================================================

    VIASTDDEF VSignal::OnSignal(VIASignal *signal, void *userData) {
        if (mCallbackFunction != nullptr) {
            mCallbackFunction(mSignalID);
        }
        return kVIA_OK;
    }

    int32_t VSignal::LookupSignal(int32_t signalID, VSignal *&signal) {
        if (gModule == nullptr) {
            return CCL_INTERNALERROR;
        }

        if (signalID < 0 || signalID >= static_cast<int32_t>(gModule->mSignal.size())) {
            return CCL_INVALIDSIGNALID;
        }

        signal = gModule->mSignal[signalID];
        if (signal == nullptr || signal->mSignalID != signalID) {
            return CCL_INVALIDSIGNALID;
        }

        return CCL_SUCCESS;
    }

#pragma endregion

#pragma region VCanMessageRequest Implementation

    // ============================================================================
    // Implementation of class VCanMessageRequest
    // ============================================================================

    VIASTDDEF VCanMessageRequest::OnMessage(VIATime time,
                                            VIAChannel channel,
                                            uint8 dir,
                                            uint32 id,
                                            uint32 flags,
                                            uint32 frameLength,
                                            VIATime startOfFrame,
                                            uint32 mBtrCfgArb,
                                            uint32 mBtrCfgData,
                                            uint32 mTimeOffsetBrsNs,
                                            uint32 mTimeOffsetCrcDelNs,
                                            uint16 mBitCount,
                                            uint32 mCRC,
                                            uint8 dataLength,
                                            const uint8 *data) {
        cclCanMessage message;
        message.time = time;
        message.channel = channel;
        message.id = id;
        message.flags =
                ((flags & kVIA_CAN_RemoteFrame) ? CCL_CANFLAGS_RTR : 0) |
                ((flags & kVIA_CAN_Wakeup) ? CCL_CANFLAGS_WAKEUP : 0) |
                ((flags & kVIA_CAN_NERR) ? CCL_CANFLAGS_TE : 0) |
                ((flags & kVIA_CAN_EDL) ? CCL_CANFLAGS_FDF : 0) |
                ((flags & kVIA_CAN_BRS) ? CCL_CANFLAGS_BRS : 0) |
                ((flags & kVIA_CAN_ESI) ? CCL_CANFLAGS_ESI : 0);
        message.dir = dir;
        message.dataLength = dataLength;
        memset(message.data, 0, 64);
        int32_t size = (flags & CCL_CANFLAGS_RTR) ? 0 : ((dataLength <= 64) ? dataLength : 64);
#if defined(_MSC_VER)
        memcpy_s(message.data, 64, data, size);
#else
        memcpy(message.data, data, size);
#endif

        if (mCallbackFunction != nullptr) {
            mCallbackFunction(&message);
        }
        return kVIA_OK;
    }

#pragma endregion

#pragma region VLinMessageRequest Implementation

    // ============================================================================
    // Implementation of class VLinMessageRequest
    // ============================================================================

    VIASTDDEF VLinMessageRequest::OnMessage(VIATime timeFrameEnd,
                                            VIATime timeFrameStart,
                                            VIATime timeHeaderEnd,
                                            VIATime timeSynchBreak,
                                            VIATime timeSynchDel,
                                            VIAChannel channel,
                                            uint8 dir,
                                            uint32 id,
                                            uint8 dlc,
                                            uint8 simulated,
                                            const uint8 data[8]) {
        cclLinFrame frame;
        frame.timestampEOF = timeFrameEnd;
        frame.timestampSOF = timeFrameStart;
        frame.timestampEOH = timeHeaderEnd;
        frame.timeSyncBreak = timeSynchBreak;
        frame.timeSyncDel = timeSynchDel;
        frame.channel = channel;
        frame.id = id;
        frame.dir = dir;
        frame.dlc = dlc;
        memset(frame.data, 0, 8);
        int32_t size = (dlc <= 8) ? dlc : 8;
#if defined(_MSC_VER)
        memcpy_s(frame.data, 8, data, size);  // use memcpy_s only to prevent security warning for Visual Studio
#else
        memcpy(frame.data, data, size);
#endif

        if (mCallbackFunction != nullptr) {
            mCallbackFunction(&frame);
        }
        return kVIA_OK;
    }

#pragma endregion

#pragma region VLinErrorRequest Implementation

    // ============================================================================
    // Implementation of class VLinErrorRequest
    // ============================================================================

    VIASTDDEF VLinErrorRequest::OnCsError(VIATime time,
                                          VIAChannel channel,
                                          uint16 crc,
                                          uint32 id,
                                          uint8 dlc,
                                          uint8 dir,
                                          uint8 FSMId,
                                          uint8 FSMState,
                                          uint8 fullTime,
                                          uint8 headerTime,
                                          uint8 simulated,
                                          const uint8 data[8]) {
        cclLinError error;
        memset(&error, 0, sizeof(cclLinError));
        error.time = time;
        error.channel = channel;
        error.id = id;
        error.type = CCL_LIN_ERROR_CHECKSUM;
        error.crc = crc;
        error.dlc = dlc;
        error.FSMId = FSMId;
        error.FSMState = FSMState;
        error.fullTime = fullTime;
        error.headerTime = headerTime;
        error.simulated = simulated;
#if defined(_MSC_VER)
        memcpy_s(error.data, 8, data, 8);  // use memcpy_s only to prevent security warning for Visual Studio
#else
        memcpy(error.data, data, 8);
#endif

        if (mCallbackFunction != nullptr) {
            mCallbackFunction(&error);
        }

        return kVIA_OK;
    }

    VIASTDDEF VLinErrorRequest::OnTransmError(VIATime time,
                                              VIAChannel channel,
                                              uint8 FSMId,
                                              uint32 id,
                                              uint8 FSMState,
                                              uint8 dlc,
                                              uint8 fullTime,
                                              uint8 headerTime) {
        cclLinError error;
        memset(&error, 0, sizeof(cclLinError));
        error.time = time;
        error.channel = channel;
        error.id = id;
        error.type = CCL_LIN_ERROR_TRANSMISSION;
        error.FSMId = FSMId;
        error.FSMState = FSMState;
        error.fullTime = fullTime;
        error.headerTime = headerTime;

        if (mCallbackFunction != nullptr) {
            mCallbackFunction(&error);
        }

        return kVIA_OK;
    }

    VIASTDDEF VLinErrorRequest::OnReceiveError(VIATime time,
                                               VIAChannel channel,
                                               uint8 FSMId,
                                               uint32 id,
                                               uint8 FSMState,
                                               uint8 dlc,
                                               uint8 fullTime,
                                               uint8 headerTime,
                                               uint8 offendingByte,
                                               uint8 shortError,
                                               uint8 stateReason,
                                               uint8 timeoutDuringDlcDetection) {
        cclLinError error;
        memset(&error, 0, sizeof(cclLinError));
        error.time = time;
        error.channel = channel;
        error.id = id;
        error.type = CCL_LIN_ERROR_RECEIVE;
        error.FSMId = FSMId;
        error.FSMState = FSMState;
        error.dlc = dlc;
        error.fullTime = fullTime;
        error.headerTime = headerTime;
        error.offendingByte = offendingByte;
        error.shortError = shortError;
        error.stateReason = stateReason;
        error.timeoutDuringDlcDetection = timeoutDuringDlcDetection;

        if (mCallbackFunction != nullptr) {
            mCallbackFunction(&error);
        }

        return kVIA_OK;
    }

    VIASTDDEF VLinErrorRequest::OnSyncError(VIATime time, VIAChannel channel) {
        cclLinError error;
        memset(&error, 0, sizeof(cclLinError));
        error.time = time;
        error.channel = channel;
        error.id = CCL_LIN_ALLMESSAGES;
        error.type = CCL_LIN_ERROR_SYNC;

        if (mCallbackFunction != nullptr) {
            mCallbackFunction(&error);
        }

        return kVIA_OK;
    }

    VIASTDDEF VLinErrorRequest::OnSlaveTimeout(VIATime time,
                                               VIAChannel channel,
                                               uint8 followState,
                                               uint8 slaveId,
                                               uint8 stateId) {
        cclLinError error;
        memset(&error, 0, sizeof(cclLinError));
        error.time = time;
        error.channel = channel;
        error.id = CCL_LIN_ALLMESSAGES;
        error.type = CCL_LIN_ERROR_SLAVETIMEOUT;
        error.followState = followState;
        error.slaveId = slaveId;
        error.stateId = stateId;

        if (mCallbackFunction != nullptr) {
            mCallbackFunction(&error);
        }

        return kVIA_OK;
    }

#pragma endregion

#pragma region VLinSlaveResponseChange Implementation

    // ============================================================================
    // Implementation of class VLinSlaveResponseChange
    // ============================================================================

    VIASTDDEF VLinSlaveResponseChange::OnLinSlaveResponseChanged(
            VIATime time,
            VIAChannel channel,
            uint8 dir,
            uint32 id,
            uint32 flags,
            uint8 dlc,
            const uint8 data[8]) {
        if (mCallbackFunction != nullptr) {
            cclLinSlaveResponse frame;
            frame.time = time;
            frame.channel = channel;
            frame.id = id;
            frame.dir = dir;
            frame.flags = flags;
            frame.dlc = dlc;
            memset(frame.data, 0, 8);
            int32_t size = (dlc <= 8) ? dlc : 8;
#if defined(_MSC_VER)
            memcpy_s(frame.data, 8, data, size);  // use memcpy_s only to prevent security warning for Visual Studio
#else
            memcpy(frame.data, data, size);
#endif

            mCallbackFunction(&frame);
        }
        return kVIA_OK;
    }

#pragma endregion

#pragma region VLinPreDispatchMessage Implementation

    // ============================================================================
    // Implementation of class VLinPreDispatchMessage
    // ============================================================================

    VIASTDDEF VLinPreDispatchMessage::OnLinPreDispatchMessage(
            VIATime time,
            VIAChannel channel,
            uint8 dir,
            uint32 id,
            uint32 flags,
            uint8 dlc,
            uint8 data[8]) {
        if (mCallbackFunction != nullptr) {
            cclLinPreDispatchMessage frame;
            frame.time = time;
            frame.channel = channel;
            frame.id = id;
            frame.dir = dir;
            frame.flags = flags;
            frame.dlc = dlc;
            memset(frame.data, 0, 8);
            int32_t size = (dlc <= 8) ? dlc : 8;
#if defined(_MSC_VER)
            memcpy_s(frame.data, 8, data, size);  // use memcpy_s only to prevent security warning for Visual Studio
#else
            memcpy(frame.data, data, size);
#endif
            mCallbackFunction(&frame);
#if defined(_MSC_VER)
            memcpy_s(data, 8, frame.data, size);  // copy modified data back to the data field passed in
#else
            memcpy(data, frame.data, size);
#endif
        }

        return kVIA_OK;
    }

#pragma endregion

#pragma region VLinSleepModeEvent Implementation

    // ============================================================================
    // Implementation of class VLinSleepModeEvent
    // ============================================================================

    VIASTDDEF VLinSleepModeEvent::OnSleepModeEvent(
            VIATime time,
            VIAChannel channel,
            uint8 external,
            uint8 isAwake,
            uint8 reason,
            uint8 wasAwake) {
        if (mCallbackFunction != nullptr) {
            cclLinSleepModeEvent event;
            event.time = time;
            event.channel = channel;
            event.external = external;
            event.isAwake = isAwake;
            event.reason = reason;
            event.wasAwake = wasAwake;
            mCallbackFunction(&event);
        }

        return kVIA_OK;
    }

#pragma endregion

#pragma region VLinWakeupFrame implementation

    // ============================================================================
    // Implementation of class VLinWakeupFrame
    // ============================================================================

    VIASTDDEF VLinWakeupFrame::OnWakeupFrame(
            VIATime time,
            VIAChannel channel,
            uint8 external,
            uint8 signal) {
        if (mCallbackFunction != nullptr) {
            cclLinWakeupFrame frame;
            frame.time = time;
            frame.channel = channel;
            frame.external = external;
            frame.signal = signal;
            mCallbackFunction(&frame);
        }

        return kVIA_OK;
    }

#pragma endregion

#pragma region VEthernetPacketRequest Implementation

    // ============================================================================
    // Implementation of class VEthernetPacketRequest
    // ============================================================================

    VIASTDDEF VEthernetPacketRequest::OnEthernetPacket(VIATime time, VIAChannel channel, uint8 dir, uint32 packetSize,
                                                       const uint8 *packetData) {
        cclEthernetPacket packet;
        packet.time = time;
        packet.channel = channel;
        packet.dir = dir;
        packet.packetSize = packetSize;

        // who is freeing this memory?
        packet.packetData = new uint8_t[packetSize];

#if defined(_MSC_VER)
        memcpy_s(packet.packetData, packetSize, packetData, packetSize);
#else
        memcpy(packet.packetData, packetData, packetSize);
#endif

        if (mCallbackFunction != nullptr) {
            mCallbackFunction(&packet);
        }
        // we have to free the memory here OR we do not allocate memory at all and pass the raw pointer to the callback function
        delete[] packet.packetData;
        return kVIA_OK;
    }

#pragma endregion

#pragma region VLinMessageRequest Implementation


#pragma region VValueBase Implementation

    // generic lookup of any type of value by ID
    template<typename T>
    int32_t IValue::LookupValue(cclValueID valueID, T *&value) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        value = dynamic_cast<T *>(gModule->mValues.Get(valueID));
        if (value == nullptr)
            return CCL_INVALIDVALUEID;

        return CCL_SUCCESS;
    }

    VValueBase::VValueBase()
            : mValueID(NDetail::cInvalidCCLObjectID), mMemberStatus(nullptr),
              mMemberHandle(kVIAFbTypeMemberHandleWhole), mCallbackFunction(nullptr), mCallbackHandle(nullptr) {
    }

    VValueBase::~VValueBase() {
        if (mCallbackHandle != nullptr) {
            if (mMemberStatus != nullptr) mMemberStatus->UnregisterObserver(mCallbackHandle);
            mCallbackHandle = nullptr;
            mCallbackFunction = nullptr;
        }
        if (mMemberStatus != nullptr) mMemberStatus->Release();
    }

    bool VValueBase::IsEqual(const IValue *other) const {
        if (other == this) return true;
        const VValueBase *value = dynamic_cast<const VValueBase *>(other);
        return (value != nullptr)
               && (mMemberHandle == value->mMemberHandle)
               && (mTypeLevel == value->mTypeLevel);
    }

    int32_t VValueBase::Init(VIAFbValue *implValue, const std::string &memberPath) {
        if (implValue != nullptr) {
            if (!memberPath.empty()) {
                NDetail::VIAObjectGuard <VIAFbType> implType;
                if (implValue->GetType(&(implType.mVIAObject)) != kVIA_OK)
                    return CCL_INTERNALERROR;

                if (implType.mVIAObject->GetMember(memberPath.c_str(), &mMemberHandle) != kVIA_OK)
                    return CCL_INVALIDMEMBERPATH;
            }

            NDetail::VIAObjectGuard <VIAFbType> memberType;
            if (implValue->GetMemberType(mMemberHandle, mTypeLevel, &(memberType.mVIAObject)) != kVIA_OK)
                return CCL_INTERNALERROR;

            if (memberType.mVIAObject->GetTypeTag(&mMemberType) != kVIA_OK)
                return CCL_INTERNALERROR;

            if (implValue->GetMemberStatus(mMemberHandle, &mMemberStatus) != kVIA_OK)
                return CCL_INTERNALERROR;
        }

        return CCL_SUCCESS;
    }

    VIASTDDEF VValueBase::OnValueUpdated(VIATime inTime, VIAFbStatus *inStatus) {
        if (mCallbackFunction != nullptr) {
            // VPortValueEntity objects are never removed during measurement, therefore their ID stays valid
            mCallbackFunction(static_cast<cclTime>(inTime), mValueID);
        }
        return kVIA_OK;
    }

    int32_t VValueBase::GetInteger(int64_t *x) const {
        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue;
        if (AccessValue(mTypeLevel, mMemberHandle, &(fbValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        if (mMemberType == VIAFbTypeTag::eSignedInteger) {
            int64 value;
            if (fbValue.mVIAObject->GetSignedInteger(kVIAFbTypeMemberHandleWhole, &value) != kVIA_OK)
                return CCL_INTERNALERROR;
            *x = value;
            return CCL_SUCCESS;
        } else if (mMemberType == VIAFbTypeTag::eUnsignedInteger) {
            uint64 value;
            if (fbValue.mVIAObject->GetUnsignedInteger(kVIAFbTypeMemberHandleWhole, &value) != kVIA_OK)
                return CCL_INTERNALERROR;
            *x = static_cast<int64_t>(value);
            return CCL_SUCCESS;
        }

        return CCL_WRONGTYPE;
    }

    int32_t VValueBase::SetInteger(int64_t x) {
        NDetail::VIAObjectGuard <VIAFbValue> fbValue;
        if (AccessValue(mTypeLevel, mMemberHandle, &(fbValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        if (mMemberType == VIAFbTypeTag::eSignedInteger) {
            int64 value{x};
            if (fbValue.mVIAObject->SetSignedInteger(kVIAFbTypeMemberHandleWhole, value) != kVIA_OK)
                return CCL_INTERNALERROR;
        } else if (mMemberType == VIAFbTypeTag::eUnsignedInteger) {
            uint64 value{static_cast<uint64>(x)};
            if (fbValue.mVIAObject->SetUnsignedInteger(kVIAFbTypeMemberHandleWhole, value) != kVIA_OK)
                return CCL_INTERNALERROR;
        } else {
            return CCL_WRONGTYPE;
        }

        return SetValueToVE(fbValue.mVIAObject);
    }

    int32_t VValueBase::GetFloat(double *x) const {
        if (mMemberType != VIAFbTypeTag::eFloat)
            return CCL_WRONGTYPE;

        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue;
        if (AccessValue(mTypeLevel, mMemberHandle, &(fbValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        if (fbValue.mVIAObject->GetFloat(kVIAFbTypeMemberHandleWhole, x) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

    int32_t VValueBase::SetFloat(double x) {
        if (mMemberType != VIAFbTypeTag::eFloat)
            return CCL_WRONGTYPE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue;
        if (AccessValue(mTypeLevel, mMemberHandle, &(fbValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        if (fbValue.mVIAObject->SetFloat(kVIAFbTypeMemberHandleWhole, x) != kVIA_OK)
            return CCL_INTERNALERROR;

        return SetValueToVE(fbValue.mVIAObject);
    }

    int32_t VValueBase::GetString(char *buffer, int32_t bufferSize) const {
        if (mMemberType != VIAFbTypeTag::eString)
            return CCL_WRONGTYPE;

        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue;
        if (AccessValue(mTypeLevel, mMemberHandle, &(fbValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        switch (fbValue.mVIAObject->GetString(kVIAFbTypeMemberHandleWhole, buffer, bufferSize)) {
            case kVIA_OK:
                return CCL_SUCCESS;
            case kVIA_BufferToSmall:
                return CCL_BUFFERTOSMALL;
            default:
                break;
        }

        return CCL_INTERNALERROR;
    }

    int32_t VValueBase::SetString(const char *str) {
        if (mMemberType != VIAFbTypeTag::eString)
            return CCL_WRONGTYPE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue;
        if (AccessValue(mTypeLevel, mMemberHandle, &(fbValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        if (fbValue.mVIAObject->SetString(kVIAFbTypeMemberHandleWhole, str) != kVIA_OK)
            return CCL_INTERNALERROR;

        return SetValueToVE(fbValue.mVIAObject);
    }

    int32_t VValueBase::GetData(uint8_t *buffer, int32_t *bufferSize) const {
        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue;
        if (AccessValue(mTypeLevel, mMemberHandle, &(fbValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        if (fbValue.mVIAObject->GetData(kVIAFbTypeMemberHandleWhole, buffer, *bufferSize) != kVIA_OK)
            return CCL_BUFFERTOSMALL;

        size_t size;
        if (fbValue.mVIAObject->GetSize(kVIAFbTypeMemberHandleWhole, &size) != kVIA_OK)
            return CCL_INTERNALERROR;
        *bufferSize = static_cast<int32_t>(size);

        return CCL_SUCCESS;
    }

    int32_t VValueBase::SetData(const uint8_t *data, int32_t size) {
        NDetail::VIAObjectGuard <VIAFbValue> fbValue;
        if (AccessValue(mTypeLevel, mMemberHandle, &(fbValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        if (fbValue.mVIAObject->SetData(kVIAFbTypeMemberHandleWhole, data, size) != kVIA_OK)
            return CCL_INVALIDDATA;

        return SetValueToVE(fbValue.mVIAObject);
    }

    int32_t VValueBase::GetEnum(int32_t *x) const {
        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue;
        if (AccessValue(mTypeLevel, mMemberHandle, &(fbValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        switch (mMemberType) {
            case VIAFbTypeTag::eSignedInteger: {
                int64 value;
                if (fbValue.mVIAObject->GetSignedInteger(kVIAFbTypeMemberHandleWhole, &value) != kVIA_OK)
                    return CCL_INTERNALERROR;
                *x = static_cast<int32_t>(value);
                return CCL_SUCCESS;
            }
            case VIAFbTypeTag::eUnsignedInteger: {
                uint64 value;
                if (fbValue.mVIAObject->GetUnsignedInteger(kVIAFbTypeMemberHandleWhole, &value) != kVIA_OK)
                    return CCL_INTERNALERROR;
                *x = static_cast<int32_t>(value);
                return CCL_SUCCESS;
            }
            default:
                break;
        }

        return CCL_WRONGTYPE;
    }

    int32_t VValueBase::SetEnum(int32_t x) {
        NDetail::VIAObjectGuard <VIAFbValue> fbValue;
        if (AccessValue(mTypeLevel, mMemberHandle, &(fbValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        switch (mMemberType) {
            case VIAFbTypeTag::eSignedInteger:
                if (fbValue.mVIAObject->SetSignedInteger(kVIAFbTypeMemberHandleWhole, static_cast<int64>(x)) != kVIA_OK)
                    return CCL_INTERNALERROR;
                break;
            case VIAFbTypeTag::eUnsignedInteger:
                if (fbValue.mVIAObject->SetUnsignedInteger(kVIAFbTypeMemberHandleWhole, static_cast<uint64>(x)) !=
                    kVIA_OK)
                    return CCL_INTERNALERROR;
                break;
            default:
                return CCL_WRONGTYPE;
        }

        return SetValueToVE(fbValue.mVIAObject);
    }

    int32_t VValueBase::GetUnionSelector(int32_t *selector) const {
        if (mMemberType != VIAFbTypeTag::eUnion)
            return CCL_WRONGTYPE;

        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue;
        if (AccessValue(mTypeLevel, mMemberHandle, &(fbValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        size_t index;
        if (fbValue.mVIAObject->GetValidMemberIndex(kVIAFbTypeMemberHandleWhole, &index) != kVIA_OK)
            return CCL_INTERNALERROR;

        *selector = static_cast<int32_t>(index);
        return CCL_SUCCESS;
    }

    int32_t VValueBase::GetArraySize(int32_t *size) const {
        if (mMemberType != VIAFbTypeTag::eArray)
            return CCL_WRONGTYPE;

        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue;
        if (AccessValue(mTypeLevel, mMemberHandle, &(fbValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        size_t length;
        if (fbValue.mVIAObject->GetArrayLength(kVIAFbTypeMemberHandleWhole, &length) != kVIA_OK)
            return CCL_INTERNALERROR;

        *size = static_cast<int32_t>(length);
        return CCL_SUCCESS;
    }

    int32_t VValueBase::SetArraySize(int32_t size) {
        if (mMemberType != VIAFbTypeTag::eArray)
            return CCL_WRONGTYPE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue;
        if (AccessValue(mTypeLevel, mMemberHandle, &(fbValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        NDetail::VIAObjectGuard <VIAFbType> fbType;
        if (fbValue.mVIAObject->GetType(&(fbType.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        VIAFbTypeTag typeTag;
        if (fbType.mVIAObject->GetTypeTag(&typeTag) != kVIA_OK)
            return CCL_INTERNALERROR;

        if (typeTag != VIAFbTypeTag::eArray)
            return CCL_WRONGTYPE;

        size_t length{static_cast<size_t>(size)};
        if (fbValue.mVIAObject->SetArrayLength(kVIAFbTypeMemberHandleWhole, length) != kVIA_OK)
            return CCL_ARRAYNOTRESIZABLE;

        return SetValueToVE(fbValue.mVIAObject);
    }

    int32_t VValueBase::GetValueType(cclValueType *valueType) const {
        switch (mMemberType) {
            case VIAFbTypeTag::eSignedInteger:
            case VIAFbTypeTag::eUnsignedInteger:
                *valueType = cclValueType::CCL_VALUE_TYPE_INTEGER;
                return CCL_SUCCESS;
            case VIAFbTypeTag::eFloat:
                *valueType = cclValueType::CCL_VALUE_TYPE_FLOAT;
                return CCL_SUCCESS;
            case VIAFbTypeTag::eString:
                *valueType = cclValueType::CCL_VALUE_TYPE_STRING;
                return CCL_SUCCESS;
            case VIAFbTypeTag::eArray:
                *valueType = cclValueType::CCL_VALUE_TYPE_ARRAY;
                return CCL_SUCCESS;
            case VIAFbTypeTag::eStruct:
                *valueType = cclValueType::CCL_VALUE_TYPE_STRUCT;
                return CCL_SUCCESS;
            case VIAFbTypeTag::eUnion:
                *valueType = cclValueType::CCL_VALUE_TYPE_UNION;
                return CCL_SUCCESS;
            default:
                break;
        }
        return CCL_UNSUPPORTEDTYPE;
    }

    int32_t VValueBase::IsValid(bool *isValid) const {
        if (isValid == nullptr)
            return CCL_PARAMETERINVALID;

        NDetail::VIAObjectGuard <VIAFbValue> fullValue;
        if (AccessValue(VIAFbTypeLevel::eImpl, kVIAFbTypeMemberHandleWhole, &(fullValue.mVIAObject)) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        if (fullValue.mVIAObject->IsValid(mMemberHandle, isValid) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

    int32_t VValueBase::ClearValue() {
        if (mMemberHandle == kVIAFbTypeMemberHandleWhole)
            return CCL_VALUENOTOPTIONAL;

        NDetail::VIAObjectGuard <VIAFbValue> fullValue;
        if (AccessValue(VIAFbTypeLevel::eImpl, kVIAFbTypeMemberHandleWhole, &(fullValue.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        if (fullValue.mVIAObject->SetIsValid(mMemberHandle, false) != kVIA_OK)
            return CCL_VALUENOTOPTIONAL;

        return SetValueToVE(fullValue.mVIAObject);
    }

    int32_t VValueBase::GetUpdateTimeNS(cclTime *time) const {
        if (mMemberStatus == nullptr)
            return CCL_INTERNALERROR;

        VIATime value;
        if (mMemberStatus->GetLastUpdateTimestamp(&value) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        *time = value;
        return CCL_SUCCESS;
    }

    int32_t VValueBase::GetChangeTimeNS(cclTime *time) const {
        if (mMemberStatus == nullptr)
            return CCL_INTERNALERROR;

        VIATime value;
        if (mMemberStatus->GetLastChangeTimestamp(&value) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        *time = value;
        return CCL_SUCCESS;
    }

    int32_t VValueBase::GetState(cclValueState *state) const {
        if (mMemberStatus == nullptr)
            return CCL_INTERNALERROR;

        VIAFbValueState valueState;
        if (mMemberStatus->GetValueState(&valueState) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        switch (valueState) {
            case VIAFbValueState::eOffline:
                *state = cclValueState::CCL_VALUE_STATE_OFFLINE;
                return CCL_SUCCESS;
            case VIAFbValueState::eMeasured:
                *state = cclValueState::CCL_VALUE_STATE_MEASURED;
                return CCL_SUCCESS;
            default:
                break;
        }

        return CCL_INTERNALERROR;
    }

    int32_t VValueBase::SetHandler(bool onUpdate, cclValueHandler handler) {
        if (mMemberStatus == nullptr)
            return CCL_INTERNALERROR;

        if (mCallbackHandle != nullptr) {
            auto rc = mMemberStatus->UnregisterObserver(mCallbackHandle);
            mCallbackHandle = nullptr;
            mCallbackFunction = nullptr;
            if (rc != kVIA_OK)
                return CCL_INTERNALERROR;
        }

        mCallbackFunction = handler;

        if (mCallbackFunction != nullptr) {
            if (mMemberStatus->RegisterObserver(this, onUpdate ? VIAFbUpdateMode::eNotifyOnUpdate
                                                               : VIAFbUpdateMode::eNotifyOnChange, &mCallbackHandle) !=
                kVIA_OK) {
                mCallbackFunction = nullptr;
                mCallbackHandle = nullptr;
                return CCL_INTERNALERROR;
            }
        }

        return CCL_SUCCESS;
    }

#pragma endregion


#pragma region VPortValueBase Implementation

    // base class for Value Entities and Call Context Values
    VPortValueBase::VPortValueBase(VIAFbValuePort *port)
            : VPortBase(port, true) {
    }

    VPortValueBase::~VPortValueBase() {
    }

    bool VPortValueBase::IsEqual(const IValue *other) const {
        if (other == this) return true;
        const VPortValueBase *value = dynamic_cast<const VPortValueBase *>(other);
        return VPortBase<VIAFbValuePort>::IsEqual(value) && VValueBase::IsEqual(value);
    }

#pragma endregion

#pragma region VPortValueEntity Implementation

    // Value Entity class
    VPortValueEntity::VPortValueEntity(VIAFbValuePort *port)
            : VPortValueBase(port) {}

    VPortValueEntity::~VPortValueEntity() {
    }

    int32_t VPortValueEntity::Init(const char *memberPath, cclValueRepresentation repr) {
        return InitImplementation(nullptr, memberPath, repr);
    }

    int32_t
    VPortValueEntity::InitPduSignal(const char *signalName, const char *memberPath, cclValueRepresentation repr) {
        return InitImplementation(signalName, memberPath, repr);
    }

    int32_t
    VPortValueEntity::InitImplementation(const char *signalName, const char *memberPath, cclValueRepresentation repr) {
        mCallbackHandle = nullptr;

        mValueClass = VIAFbValueClass::ePortValue;
        mMemberHandle = kVIAFbTypeMemberHandleWhole;
        mSignalName = std::string{};

        std::string remaining{(memberPath != nullptr) ? memberPath : ""};
        if (!remaining.empty()) {
            // leading . is optional
            if (remaining.front() == '.')
                remaining.erase(0, 1);

            if (!remaining.empty()) {
                // customer may escape keywords
                if (remaining.front() == '#')
                    remaining.erase(0, 1);
                else
                    NDetail::ExtractValueClass(remaining, mValueClass);
            }
        }

        if (!NDetail::MapTypeLevel(repr, mTypeLevel))
            return CCL_PARAMETERINVALID;

        NDetail::VIAObjectGuard <VIAFbValue> implValue;
        if (signalName == nullptr) {
            if (mPort->GetValue(VIAFbTypeLevel::eImpl, kVIAFbTypeMemberHandleWhole, mValueClass,
                                &(implValue.mVIAObject)) != kVIA_OK)
                return CCL_INVALIDMEMBERPATH;
        } else if (mValueClass != VIAFbValueClass::ePortValue) {
            return CCL_PARAMETERINVALID;
        } else {
            if (mPort->GetPDUSignalValue(VIAFbTypeLevel::eImpl, signalName, kVIAFbTypeMemberHandleWhole,
                                         &(implValue.mVIAObject)) != kVIA_OK)
                return CCL_INVALIDMEMBERPATH;
            mSignalName = signalName;
        }

        return VValueBase::Init(implValue.mVIAObject, remaining);
    }

    int32_t
    VPortValueEntity::CreateValueEntity(const char *portPath, const char *memberPath, cclValueRepresentation repr) {
        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbValuePort *port;
        if (fbAPI->GetValuePort(portPath, &port) != kVIA_OK) {
            // PDU signal support
            const char *lastPoint = strrchr(portPath, '.');
            if (lastPoint != nullptr) {
                std::string possiblePduPath{portPath, lastPoint};
                if (fbAPI->GetValuePort(possiblePduPath.c_str(), &port) == kVIA_OK) {
                    std::unique_ptr<VPortValueEntity> valueEntity{new VPortValueEntity(port)};
                    int32_t result = valueEntity->InitPduSignal(lastPoint + 1, memberPath, repr);
                    if (result != CCL_SUCCESS)
                        return result;

                    return gModule->mValues.Add(valueEntity.release());
                }
            }
            return CCL_INVALIDPORTPATH;
        }

        std::unique_ptr<VPortValueEntity> valueEntity{new VPortValueEntity(port)};
        int32_t result = valueEntity->Init(memberPath, repr);
        if (result != CCL_SUCCESS)
            return result;

        return gModule->mValues.Add(valueEntity.release());
    }

    int32_t
    VPortValueEntity::CreateValueEntity(VIAFbValuePort *port, const char *memberPath, cclValueRepresentation repr) {
        std::unique_ptr<VPortValueEntity> valueEntity{new VPortValueEntity(port)};
        int32_t result = valueEntity->Init(memberPath, repr);
        if (result != CCL_SUCCESS)
            return result;

        return gModule->mValues.Add(valueEntity.release());
    }

    inline VIAResult VPortValueEntity::AccessValue(VIAFbTypeLevel typeLevel, VIAFbTypeMemberHandle memberHandle,
                                                   VIAFbValue **value) const {
        if (mPort == nullptr) {
            return kVIA_ObjectInvalid;
        }
        if (mSignalName.empty()) {
            return mPort->GetValue(typeLevel, memberHandle, mValueClass, value);
        } else {
            return mPort->GetPDUSignalValue(typeLevel, mSignalName.c_str(), memberHandle, value);
        }
    }

    bool VPortValueEntity::IsMemberValid() const {
        if (mPort == nullptr)
            return false;

        if (mMemberHandle == kVIAFbTypeMemberHandleWhole)
            return true;

        NDetail::VIAObjectGuard <VIAFbValue> implValue;
        if (AccessValue(VIAFbTypeLevel::eImpl, kVIAFbTypeMemberHandleWhole, &(implValue.mVIAObject)) != kVIA_OK)
            return false;

        bool isValid;
        if (implValue.mVIAObject->IsValid(mMemberHandle, &isValid) != kVIA_OK)
            return false;

        return isValid;
    }

    int32_t VPortValueEntity::SetValueToVE(VIAFbValue *pValue) {
        if (mPort == nullptr) return CCL_INTERNALERROR;
        return mPort->SetValue(pValue) == kVIA_OK ? CCL_SUCCESS : CCL_VALUEREADONLY;
    }

    bool VPortValueEntity::IsEqual(const IValue *other) const {
        if (other == this) return true;
        const VPortValueEntity *ve = dynamic_cast<const VPortValueEntity *>(other);
        return (ve != nullptr)
               && VPortValueBase::IsEqual(ve)
               && ve->mValueClass == mValueClass
               && ve->mSignalName == mSignalName;
    }

#pragma endregion

#pragma region VDOValueEntity Implementation

    VDOMemberBase::~VDOMemberBase() {
        if (mMember)
            mMember->Release();
    }

    uint64_t VDOMemberBase::GetPortID() const {
        if (!mMember)
            return 0;

        uint64_t id = 0;
        auto ret = mMember->GetUniqueMemberID(&id);
        if (ret != kVIA_OK)
            return 0;

        return id;
    }

    int32_t VDOMemberBase::GetDOMemberByPath(const char *inPath, VIAFbDOMember **outDOMember) {
        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        const char *dot = strchr(inPath, '.');
        if (dot == nullptr)
            return CCL_INVALIDPORTPATH;

        std::string doPath{inPath, dot};
        VIAFbDO *pDO = nullptr;
        auto ret = fbAPI->GetDO(doPath.c_str(), &pDO);
        if (ret != kVIA_OK || pDO == nullptr)
            return CCL_INVALIDPORTPATH;

        ret = pDO->GetMemberByPath(dot + 1, outDOMember);
        pDO->Release();
        if (ret != kVIA_OK || *outDOMember == nullptr)
            return CCL_INVALIDPORTPATH;

        return CCL_SUCCESS;
    }

    VDOValueEntity::~VDOValueEntity() {
    }

    int32_t
    VDOValueEntity::CreateValueEntity(const char *portPath, const char *memberPath, cclValueRepresentation repr) {
        VIAFbDOMember *pMember = nullptr;
        auto result = GetDOMemberByPath(portPath, &pMember);
        if (result != CCL_SUCCESS)
            return result;
        auto valueEntity = std::unique_ptr<VDOValueEntity>{new VDOValueEntity{pMember}};
        result = valueEntity->Init(memberPath, repr);
        if (result != CCL_SUCCESS)
            return result;

        return gModule->mValues.Add(valueEntity.release());
    }

    bool VDOValueEntity::IsEqual(const IValue *other) const {
        if (other == this) return true;
        auto ve = dynamic_cast<const VDOValueEntity *>(other);
        return (ve != nullptr)
               && VValueBase::IsEqual(ve)
               && mValueClass == ve->mValueClass;
    }

    VIAResult VDOValueEntity::AccessValue(VIAFbTypeLevel typeLevel, VIAFbTypeMemberHandle memberHandle,
                                          VIAFbValue **value) const {
        if (!mMember)
            return kVIA_ObjectInvalid;
        switch (mValueClass) {
            case ePortValue: {
                VIAFbDOValue *pDOValue = nullptr;
                auto ret = VIAQueryFbDOMemberInterface(mMember, &pDOValue);
                if (ret != kVIA_OK) return ret;
                ret = pDOValue->GetValue(typeLevel, memberHandle, value);
                pDOValue->Release();
                return ret;
            }
            case eSubscriptionState: {
                VIAFbDOSubscribable *pDOSubscribable = nullptr;
                auto ret = VIAQueryFbDOMemberInterface(mMember, &pDOSubscribable);
                if (ret != kVIA_OK) return ret;
                ret = pDOSubscribable->GetSubscriptionState(value);
                pDOSubscribable->Release();
                return ret;
            }
            case eAnnouncementState: {
                VIAFbDOAnnounceable *pDOAnnounceable = nullptr;
                auto ret = VIAQueryFbDOMemberInterface(mMember, &pDOAnnounceable);
                if (ret != kVIA_OK) return ret;
                ret = pDOAnnounceable->GetAnnouncementState(value);
                pDOAnnounceable->Release();
                return ret;
            }
            case eLatestCall: {
                VIAFbDOMethod *pDOMethod = nullptr;
                auto ret = VIAQueryFbDOMemberInterface(mMember, &pDOMethod);
                if (ret != kVIA_OK) return ret;
                ret = pDOMethod->GetLatestCall(typeLevel, value);
                pDOMethod->Release();
                if (ret != kVIA_OK) return ret;
                if (memberHandle != kVIAFbTypeMemberHandleWhole) {
                    VIAFbValue *memberValue = nullptr;
                    ret = (*value)->GetMemberValue(memberHandle, typeLevel, &memberValue);
                    (*value)->Release();
                    *value = nullptr;
                    if (ret != kVIA_OK) return ret;
                    *value = memberValue;
                }
                return ret;
            }
            case eLatestReturn: {
                VIAFbDOMethod *pDOMethod = nullptr;
                auto ret = VIAQueryFbDOMemberInterface(mMember, &pDOMethod);
                if (ret != kVIA_OK) return ret;
                ret = pDOMethod->GetLatestReturn(typeLevel, value);
                pDOMethod->Release();
                if (ret != kVIA_OK) return ret;
                if (memberHandle != kVIAFbTypeMemberHandleWhole) {
                    VIAFbValue *memberValue = nullptr;
                    ret = (*value)->GetMemberValue(memberHandle, typeLevel, &memberValue);
                    (*value)->Release();
                    *value = nullptr;
                    if (ret != kVIA_OK) return ret;
                    *value = memberValue;
                }
                return ret;
            }
            case eParamDefaults: {
                VIAFbDOMethodProvider *pDOMethod = nullptr;
                auto ret = VIAQueryFbDOMemberInterface(mMember, &pDOMethod);
                if (ret != kVIA_OK) return ret;
                ret = pDOMethod->GetDefaultOutParamsValue(typeLevel, value);
                pDOMethod->Release();
                if (ret != kVIA_OK) return ret;
                if (memberHandle != kVIAFbTypeMemberHandleWhole) {
                    VIAFbValue *memberValue = nullptr;
                    ret = (*value)->GetMemberValue(memberHandle, typeLevel, &memberValue);
                    (*value)->Release();
                    *value = nullptr;
                    if (ret != kVIA_OK) return ret;
                    *value = memberValue;
                }
                return ret;
            }
            default:
                return kVIA_Failed;
        }
    }

    bool VDOValueEntity::IsMemberValid() const {
        if (mMember == nullptr)
            return false;

        if (mMemberHandle == kVIAFbTypeMemberHandleWhole)
            return true;

        NDetail::VIAObjectGuard <VIAFbValue> implValue;
        if (AccessValue(VIAFbTypeLevel::eImpl, kVIAFbTypeMemberHandleWhole, &(implValue.mVIAObject)) != kVIA_OK)
            return false;

        bool isValid;
        if (implValue.mVIAObject->IsValid(mMemberHandle, &isValid) != kVIA_OK)
            return false;

        return isValid;
    }

    int32_t VDOValueEntity::SetValueToVE(VIAFbValue *pValue) {
        if (mMember == nullptr)
            return CCL_INTERNALERROR;

        switch (mValueClass) {
            case ePortValue: {
                VIAFbDOValueProvider *pDOValueProvider = nullptr;
                auto ret = VIAQueryFbDOMemberInterface(mMember, &pDOValueProvider);
                if (ret != kVIA_OK) return ret;
                ret = pDOValueProvider->SetValue(pValue);
                pDOValueProvider->Release();
                return ret == kVIA_OK ? CCL_SUCCESS : CCL_VALUEREADONLY;
            }
            case eSubscriptionState:
            case eAnnouncementState:
            case eLatestCall:
            case eLatestReturn:
                return CCL_VALUEREADONLY;
            case eParamDefaults: {
                auto ret = pValue->Commit();
                return ret == kVIA_OK ? CCL_SUCCESS : CCL_VALUEREADONLY;
            }
            default:
                return kVIA_Failed;
        }
    }

    VDOValueEntity::VDOValueEntity(VIAFbDOMember *pDOMember)
            : VValueBase(), VDOMemberBase(pDOMember), mValueClass(ePortValue) {
    }

    int32_t VDOValueEntity::Init(const char *memberPath, cclValueRepresentation repr) {
        mCallbackHandle = nullptr;

        mValueClass = VIAFbValueClass::ePortValue;
        mMemberHandle = kVIAFbTypeMemberHandleWhole;

        std::string remaining{(memberPath != nullptr) ? memberPath : ""};
        if (!remaining.empty()) {
            // leading . is optional
            if (remaining.front() == '.')
                remaining.erase(0, 1);

            if (!remaining.empty()) {
                // customer may escape keywords
                if (remaining.front() == '#')
                    remaining.erase(0, 1);
                else
                    NDetail::ExtractValueClass(remaining, mValueClass);
            }
        }

        if (!NDetail::MapTypeLevel(repr, mTypeLevel))
            return CCL_PARAMETERINVALID;

        NDetail::VIAObjectGuard <VIAFbValue> implValue;
        if (AccessValue(VIAFbTypeLevel::eImpl, kVIAFbTypeMemberHandleWhole, &(implValue.mVIAObject)) != kVIA_OK)
            return CCL_INVALIDMEMBERPATH; // e.g. mValueClass invalid for the member type or communication pattern

        return VValueBase::Init(implValue.mVIAObject, remaining);
    }

#pragma endregion

#pragma region VCallContextValue Implementation

    // Call Context Value class
    VCallContextValue::VCallContextValue()
            : VPortValueBase(nullptr), mCallContextID(NDetail::cInvalidCCLObjectID) {}

    VCallContextValue::~VCallContextValue() {}

    int32_t VCallContextValue::Init(cclCallContextID ctxtID, const char *memberPath, cclValueRepresentation repr) {
        mCallContextID = ctxtID;

        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_INVALIDCALLCONTEXTID;

        if (!NDetail::MapTypeLevel(repr, mTypeLevel))
            return CCL_PARAMETERINVALID;

        std::string remaining{(memberPath != nullptr) ? memberPath : ""};
        if (remaining.empty())
            return CCL_INVALIDMEMBERPATH;

        // leading . is optional
        if (remaining.front() == '.') remaining.erase(0, 1);
        NDetail::ExtractValueClass(remaining, mValueClass);

        NDetail::VIAObjectGuard <VIAFbValue> implValue;
        switch (mValueClass) {
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INVALIDMEMBERPATH;

            case NDetail::CallContextValueClass::eInParam:
                if (ctxt->GetInParams(VIAFbTypeLevel::eImpl, &(implValue.mVIAObject)) != kVIA_OK)
                    return CCL_INTERNALERROR;
                break;

            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                if (ctxt->GetOutParams(VIAFbTypeLevel::eImpl, &(implValue.mVIAObject)) != kVIA_OK)
                    return CCL_INTERNALERROR;
                break;

            default:
                return remaining.empty() ? CCL_SUCCESS : CCL_INVALIDMEMBERPATH;
        }

        return VValueBase::Init(implValue.mVIAObject, remaining);
    }

    int32_t VCallContextValue::CreateCallContextValue(cclCallContextID ctxtID, const char *memberPath,
                                                      cclValueRepresentation repr) {
        std::unique_ptr<VCallContextValue> callContextValue{new VCallContextValue()};
        int32_t result = callContextValue->Init(ctxtID, memberPath, repr);
        if (result != CCL_SUCCESS)
            return result;

        return gModule->mValues.Add(callContextValue.release());
    }

    VIAFbCallContext *VCallContextValue::GetCallContext() const {
        // call context may have expired, always obtain the pointer from global map
        VCallContext *callContext = gModule->mCallContexts.Get(mCallContextID);
        return (callContext != nullptr) ? callContext->mContext : nullptr;
    }

    VIAFbValue *VCallContextValue::GetParamsValue(VIAFbCallContext *ctxt, bool *isValidMember) const {
        NDetail::VIAObjectGuard <VIAFbValue> implValue;
        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
                if (ctxt->GetInParams(VIAFbTypeLevel::eImpl, &(implValue.mVIAObject)) != kVIA_OK) return nullptr;
                break;

            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                if (ctxt->GetOutParams(VIAFbTypeLevel::eImpl, &(implValue.mVIAObject)) != kVIA_OK) return nullptr;
                break;

            default:
                return nullptr;
        }

        if (implValue.mVIAObject->IsValid(mMemberHandle, isValidMember) != kVIA_OK)
            return nullptr;

        VIAFbValue *memberValue;
        if (implValue.mVIAObject->GetMemberValue(mMemberHandle, mTypeLevel, &memberValue) != kVIA_OK)
            return nullptr;

        return memberValue;
    }

    int32_t VCallContextValue::SetParamsValue(VIAFbCallContext *ctxt, VIAFbValue *value) const {
        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
                if (ctxt->SetInParams(value) != kVIA_OK)
                    return CCL_WRONGSTATE;
                break;

            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                if (ctxt->SetOutParams(value) != kVIA_OK)
                    return CCL_WRONGSTATE;
                break;

            default:
                break;
        }
        return CCL_SUCCESS;
    }

    VIAResult VCallContextValue::AccessValue(VIAFbTypeLevel typeLevel, VIAFbTypeMemberHandle memberHandle,
                                             VIAFbValue **value) const {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return kVIA_ObjectInvalid;
        bool isValid;
        *value = GetParamsValue(ctxt, &isValid);
        if (*value == nullptr)
            return kVIA_ObjectInvalid;
        return kVIA_OK;
    }

    bool VCallContextValue::IsMemberValid() const {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return false;
        bool isValid = false;
        VIAFbValue *value = GetParamsValue(ctxt, &isValid);
        return isValid;
    }

    int32_t VCallContextValue::SetValueToVE(VIAFbValue *pValue) {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        return SetParamsValue(ctxt, pValue);
    }

    int32_t VCallContextValue::GetInteger(int64_t *x) const {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::GetInteger(x);
            case NDetail::CallContextValueClass::eCallTime: {
                VIATime time;
                if (ctxt->GetCallTime(&time) != kVIA_OK)
                    return CCL_INTERNALERROR;
                if (time == kVIAFbTimeUndefined)
                    return CCL_INVALIDCALLCONTEXTSTATE;
                *x = time;
                return CCL_SUCCESS;
            }
            case NDetail::CallContextValueClass::eReturnTime: {
                VIATime time;
                if (ctxt->GetReturnTime(&time) != kVIA_OK)
                    return CCL_INTERNALERROR;
                if (time == kVIAFbTimeUndefined)
                    return CCL_INVALIDCALLCONTEXTSTATE;
                *x = time;
                return CCL_SUCCESS;
            }
            case NDetail::CallContextValueClass::eRequestID:
                return (ctxt->GetRequestID(x) != kVIA_OK) ? CCL_INTERNALERROR : CCL_SUCCESS;
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::SetInteger(int64_t x) {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::SetInteger(x);
            case NDetail::CallContextValueClass::eCallTime:
            case NDetail::CallContextValueClass::eReturnTime:
            case NDetail::CallContextValueClass::eRequestID:
                return CCL_VALUEREADONLY;
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::GetFloat(double *x) const {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::GetFloat(x);
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::SetFloat(double x) {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::SetFloat(x);
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::GetString(char *buffer, int32_t bufferSize) const {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::GetString(buffer, bufferSize);
            case NDetail::CallContextValueClass::eConsumerPort:
                return (ctxt->GetClientPort(buffer, bufferSize) != kVIA_OK) ? CCL_INVALIDSIDE : CCL_SUCCESS;
            case NDetail::CallContextValueClass::eProviderPort:
                return (ctxt->GetServerPort(buffer, bufferSize) != kVIA_OK) ? CCL_INVALIDSIDE : CCL_SUCCESS;
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::SetString(const char *str) {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::SetString(str);
            case NDetail::CallContextValueClass::eConsumerPort:
            case NDetail::CallContextValueClass::eProviderPort:
                return CCL_VALUEREADONLY;
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }

        if (mMemberType != VIAFbTypeTag::eString)
            return CCL_WRONGTYPE;
    }

    int32_t VCallContextValue::GetData(uint8_t *buffer, int32_t *bufferSize) const {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::GetData(buffer, bufferSize);
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::SetData(const uint8_t *data, int32_t size) {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::SetData(data, size);
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::GetEnum(int32_t *x) const {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::GetEnum(x);
            case NDetail::CallContextValueClass::eSide: {
                VIAFbFunctionCallSide side;
                if (ctxt->GetSide(&side) != kVIA_OK)
                    return CCL_INTERNALERROR;
                *x = static_cast<int32_t>((side == VIAFbFunctionCallSide::eClient) ? cclSide::CCL_SIDE_CONSUMER
                                                                                   : cclSide::CCL_SIDE_PROVIDER);
                return CCL_SUCCESS;
            }
            case NDetail::CallContextValueClass::eCallState: {
                VIAFbFunctionCallState state;
                if (ctxt->GetCallState(&state) != kVIA_OK)
                    return CCL_INTERNALERROR;
                cclCallState mapped;
                if (VFunctionUtils::MapCallState(state, mapped) < 0)
                    return CCL_INVALIDCALLCONTEXTSTATE;
                *x = static_cast<int32_t>(mapped);
                return CCL_SUCCESS;
            }
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::SetEnum(int32_t x) {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::SetEnum(x);
            case NDetail::CallContextValueClass::eSide:
            case NDetail::CallContextValueClass::eCallState:
                return CCL_VALUEREADONLY;
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::GetUnionSelector(int32_t *selector) const {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::GetUnionSelector(selector);
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::GetArraySize(int32_t *size) const {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::GetArraySize(size);
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::SetArraySize(int32_t size) {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::SetArraySize(size);
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::GetValueType(cclValueType *valueType) const {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::GetValueType(valueType);
            case NDetail::CallContextValueClass::eCallTime:
            case NDetail::CallContextValueClass::eReturnTime:
            case NDetail::CallContextValueClass::eRequestID:
                *valueType = cclValueType::CCL_VALUE_TYPE_INTEGER;
                return CCL_SUCCESS;
            case NDetail::CallContextValueClass::eSide:
            case NDetail::CallContextValueClass::eCallState:
                *valueType = cclValueType::CCL_VALUE_TYPE_ENUM;
                return CCL_SUCCESS;
            case NDetail::CallContextValueClass::eConsumerPort:
            case NDetail::CallContextValueClass::eProviderPort:
                *valueType = cclValueType::CCL_VALUE_TYPE_STRING;
                return CCL_SUCCESS;
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::IsValid(bool *isValid) const {
        if (isValid == nullptr)
            return CCL_PARAMETERINVALID;

        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::IsValid(isValid);
            case NDetail::CallContextValueClass::eCallTime:
            case NDetail::CallContextValueClass::eReturnTime:
            case NDetail::CallContextValueClass::eRequestID:
            case NDetail::CallContextValueClass::eSide:
            case NDetail::CallContextValueClass::eCallState:
            case NDetail::CallContextValueClass::eConsumerPort:
            case NDetail::CallContextValueClass::eProviderPort:
                return CCL_VALUENOTOPTIONAL;
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

    int32_t VCallContextValue::ClearValue() {
        auto *ctxt = GetCallContext();
        if (ctxt == nullptr)
            return CCL_CALLCONTEXTEXPIRED;

        switch (mValueClass) {
            case NDetail::CallContextValueClass::eInParam:
            case NDetail::CallContextValueClass::eOutParam:
            case NDetail::CallContextValueClass::eResultValue:
                return VValueBase::ClearValue();
            case NDetail::CallContextValueClass::eCallTime:
            case NDetail::CallContextValueClass::eReturnTime:
            case NDetail::CallContextValueClass::eRequestID:
            case NDetail::CallContextValueClass::eSide:
            case NDetail::CallContextValueClass::eCallState:
            case NDetail::CallContextValueClass::eConsumerPort:
            case NDetail::CallContextValueClass::eProviderPort:
                return CCL_VALUEREADONLY;
            case NDetail::CallContextValueClass::eUnknown:
                return CCL_INTERNALERROR;
            default:
                return CCL_WRONGTYPE;
        }
    }

#pragma endregion

#pragma region VPDUSignalValue Implementation

    template<typename PortType>
    VPDUSignalValue<PortType>::VPDUSignalValue(PortType *port)
            : VPortBase<PortType>(port, false), mValueID(NDetail::cInvalidCCLObjectID), mMemberStatus(nullptr),
              mMemberHandle(kVIAFbTypeMemberHandleWhole), mCallbackFunction(nullptr), mCallbackHandle(nullptr) {}

    template<typename PortType>
    VPDUSignalValue<PortType>::~VPDUSignalValue() {
        if (mCallbackHandle != nullptr) {
            if (mMemberStatus != nullptr) mMemberStatus->UnregisterObserver(mCallbackHandle);
            mCallbackHandle = nullptr;
            mCallbackFunction = nullptr;
        }
        if (mMemberStatus != nullptr) mMemberStatus->Release();
    }

    template<typename PortType>
    VIAFbValue *VPDUSignalValue<PortType>::GetMemberValue() const {
        if (mMemberHandle == kVIAFbTypeMemberHandleWhole) {
            NDetail::VIAObjectGuard <VIAFbValue> signalValue;
            if (this->mPort->GetSignalValue(mSignalName.c_str(), mTypeLevel, &(signalValue.mVIAObject)) != kVIA_OK)
                return nullptr;

            return signalValue.Release();
        }

        NDetail::VIAObjectGuard <VIAFbValue> signalValue;
        if (this->mPort->GetSignalValue(mSignalName.c_str(), VIAFbTypeLevel::eImpl, &(signalValue.mVIAObject)) !=
            kVIA_OK)
            return nullptr;

        NDetail::VIAObjectGuard <VIAFbValue> memberValue;
        if (signalValue.mVIAObject->GetMemberValue(mMemberHandle, mTypeLevel, &(memberValue.mVIAObject)) != kVIA_OK)
            return nullptr;

        return memberValue.Release();
    }

    template<typename PortType>
    bool VPDUSignalValue<PortType>::IsMemberValid() const {
        if (this->mPort == nullptr)
            return false;

        if (mMemberHandle == kVIAFbTypeMemberHandleWhole)
            return true;

        NDetail::VIAObjectGuard <VIAFbValue> signalValue;
        if (this->mPort->GetSignalValue(mSignalName.c_str(), VIAFbTypeLevel::eImpl, &(signalValue.mVIAObject)) !=
            kVIA_OK)
            return false;

        bool isValid;
        if (signalValue.mVIAObject->IsValid(mMemberHandle, &isValid) != kVIA_OK)
            return false;

        return isValid;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::Init(const char *signalName, const char *member, cclValueRepresentation repr) {
        if (signalName == nullptr)
            return CCL_PARAMETERINVALID;

        if (!NDetail::MapTypeLevel(repr, mTypeLevel))
            return CCL_PARAMETERINVALID;

        NDetail::VIAObjectGuard <VIAFbValue> signalValue;
        if (this->mPort->GetSignalValue(signalName, VIAFbTypeLevel::eImpl, &(signalValue.mVIAObject)) != kVIA_OK)
            return CCL_INVALIDSIGNALNAME;

        mSignalName = signalName;
        if ((member != nullptr) && (strlen(member) > 0)) {
            NDetail::VIAObjectGuard <VIAFbType> signalType;
            if (signalValue.mVIAObject->GetType(&(signalType.mVIAObject)) != kVIA_OK)
                return CCL_INTERNALERROR;

            if (signalType.mVIAObject->GetMember(member, &mMemberHandle) != kVIA_OK)
                return CCL_INVALIDMEMBERPATH;
        }

        NDetail::VIAObjectGuard <VIAFbType> memberType;
        if (signalValue.mVIAObject->GetMemberType(mMemberHandle, mTypeLevel, &(memberType.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        if (memberType.mVIAObject->GetTypeTag(&mMemberType) != kVIA_OK)
            return CCL_INTERNALERROR;

        if (signalValue.mVIAObject->GetMemberStatus(mMemberHandle, &mMemberStatus) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::CreateSignalValue(PortType *port, const char *signalName, const char *member,
                                                         cclValueRepresentation repr) {
        std::unique_ptr<VPDUSignalValue> signalValue{new VPDUSignalValue(port)};
        int32_t result = signalValue->Init(signalName, member, repr);
        if (result != CCL_SUCCESS)
            return result;

        return gModule->mValues.Add(signalValue.release());
    }

    template<typename PortType>
    bool VPDUSignalValue<PortType>::IsEqual(const IValue *other) const {
        if (other == this) return true;
        const VPDUSignalValue <PortType> *value = dynamic_cast<const VPDUSignalValue <PortType> *>(other);
        return (value != nullptr)
               && (VPortBase<PortType>::IsEqual(value))
               && (mMemberHandle == value->mMemberHandle)
               && (mTypeLevel == value->mTypeLevel)
               && (mSignalName == value->mSignalName);
    }

    template<typename PortType>
    VIASTDDEF VPDUSignalValue<PortType>::OnValueUpdated(VIATime inTime, VIAFbStatus *inStatus) {
        if (mCallbackFunction != nullptr) {
            // VPDUSignalValue objects are never removed during measurement, therefore their ID stays valid
            mCallbackFunction(static_cast<cclTime>(inTime), mValueID);
        }
        return kVIA_OK;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::GetInteger(int64_t *x) const {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue(GetMemberValue());
        if (fbValue.mVIAObject == nullptr)
            return CCL_VALUENOTACCESSIBLE;

        if (mMemberType == VIAFbTypeTag::eSignedInteger) {
            int64 value;
            if (fbValue.mVIAObject->GetSignedInteger(kVIAFbTypeMemberHandleWhole, &value) != kVIA_OK)
                return CCL_INTERNALERROR;
            *x = value;
            return CCL_SUCCESS;
        } else if (mMemberType == VIAFbTypeTag::eUnsignedInteger) {
            uint64 value;
            if (fbValue.mVIAObject->GetUnsignedInteger(kVIAFbTypeMemberHandleWhole, &value) != kVIA_OK)
                return CCL_INTERNALERROR;
            *x = static_cast<int64_t>(value);
            return CCL_SUCCESS;
        }

        return CCL_WRONGTYPE;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::SetInteger(int64_t x) {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue(GetMemberValue());
        if (fbValue.mVIAObject == nullptr)
            return CCL_VALUENOTACCESSIBLE;

        if (mMemberType == VIAFbTypeTag::eSignedInteger) {
            int64 value{x};
            if (fbValue.mVIAObject->SetSignedInteger(kVIAFbTypeMemberHandleWhole, value) != kVIA_OK)
                return CCL_INTERNALERROR;
        } else if (mMemberType == VIAFbTypeTag::eUnsignedInteger) {
            uint64 value{static_cast<uint64>(x)};
            if (fbValue.mVIAObject->SetUnsignedInteger(kVIAFbTypeMemberHandleWhole, value) != kVIA_OK)
                return CCL_INTERNALERROR;
        } else {
            return CCL_WRONGTYPE;
        }

        if (fbValue.mVIAObject->Commit() != kVIA_OK)
            return CCL_VALUEREADONLY;

        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::GetFloat(double *x) const {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        if (mMemberType != VIAFbTypeTag::eFloat)
            return CCL_WRONGTYPE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue(GetMemberValue());
        if (fbValue.mVIAObject == nullptr)
            return CCL_VALUENOTACCESSIBLE;

        if (fbValue.mVIAObject->GetFloat(kVIAFbTypeMemberHandleWhole, x) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::SetFloat(double x) {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        if (mMemberType != VIAFbTypeTag::eFloat)
            return CCL_WRONGTYPE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue(GetMemberValue());
        if (fbValue.mVIAObject == nullptr)
            return CCL_VALUENOTACCESSIBLE;

        if (fbValue.mVIAObject->SetFloat(kVIAFbTypeMemberHandleWhole, x) != kVIA_OK)
            return CCL_INTERNALERROR;

        if (fbValue.mVIAObject->Commit() != kVIA_OK)
            return CCL_VALUEREADONLY;

        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::GetString(char *buffer, int32_t bufferSize) const {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        if (mMemberType != VIAFbTypeTag::eString)
            return CCL_WRONGTYPE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue(GetMemberValue());
        if (fbValue.mVIAObject == nullptr)
            return CCL_VALUENOTACCESSIBLE;

        switch (fbValue.mVIAObject->GetString(kVIAFbTypeMemberHandleWhole, buffer, bufferSize)) {
            case kVIA_OK:
                return CCL_SUCCESS;
            case kVIA_BufferToSmall:
                return CCL_BUFFERTOSMALL;
            default:
                break;
        }

        return CCL_INTERNALERROR;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::SetString(const char *str) {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        if (mMemberType != VIAFbTypeTag::eString)
            return CCL_WRONGTYPE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue(GetMemberValue());
        if (fbValue.mVIAObject == nullptr)
            return CCL_VALUENOTACCESSIBLE;

        if (fbValue.mVIAObject->SetString(kVIAFbTypeMemberHandleWhole, str) != kVIA_OK)
            return CCL_INTERNALERROR;

        if (fbValue.mVIAObject->Commit() != kVIA_OK)
            return CCL_VALUEREADONLY;

        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::GetData(uint8_t *buffer, int32_t *bufferSize) const {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue(GetMemberValue());
        if (fbValue.mVIAObject == nullptr)
            return CCL_VALUENOTACCESSIBLE;

        if (fbValue.mVIAObject->GetData(kVIAFbTypeMemberHandleWhole, buffer, *bufferSize) != kVIA_OK)
            return CCL_BUFFERTOSMALL;

        size_t size;
        if (fbValue.mVIAObject->GetSize(kVIAFbTypeMemberHandleWhole, &size) != kVIA_OK)
            return CCL_INTERNALERROR;
        *bufferSize = static_cast<int32_t>(size);

        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::SetData(const uint8_t *data, int32_t size) {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue(GetMemberValue());
        if (fbValue.mVIAObject == nullptr)
            return CCL_VALUENOTACCESSIBLE;

        if (fbValue.mVIAObject->SetData(kVIAFbTypeMemberHandleWhole, data, size) != kVIA_OK)
            return CCL_INVALIDDATA;

        if (fbValue.mVIAObject->Commit() != kVIA_OK)
            return CCL_VALUEREADONLY;

        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::GetEnum(int32_t *x) const {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue(GetMemberValue());
        if (fbValue.mVIAObject == nullptr)
            return CCL_VALUENOTACCESSIBLE;

        switch (mMemberType) {
            case VIAFbTypeTag::eSignedInteger: {
                int64 value;
                if (fbValue.mVIAObject->GetSignedInteger(kVIAFbTypeMemberHandleWhole, &value) != kVIA_OK)
                    return CCL_INTERNALERROR;
                *x = static_cast<int32_t>(value);
                return CCL_SUCCESS;
            }
            case VIAFbTypeTag::eUnsignedInteger: {
                uint64 value;
                if (fbValue.mVIAObject->GetUnsignedInteger(kVIAFbTypeMemberHandleWhole, &value) != kVIA_OK)
                    return CCL_INTERNALERROR;
                *x = static_cast<int32_t>(value);
                return CCL_SUCCESS;
            }
            default:
                break;
        }

        return CCL_WRONGTYPE;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::SetEnum(int32_t x) {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue(GetMemberValue());
        if (fbValue.mVIAObject == nullptr)
            return CCL_VALUENOTACCESSIBLE;

        switch (mMemberType) {
            case VIAFbTypeTag::eSignedInteger:
                if (fbValue.mVIAObject->SetSignedInteger(kVIAFbTypeMemberHandleWhole, static_cast<int64>(x)) != kVIA_OK)
                    return CCL_INTERNALERROR;
                break;
            case VIAFbTypeTag::eUnsignedInteger:
                if (fbValue.mVIAObject->SetUnsignedInteger(kVIAFbTypeMemberHandleWhole, static_cast<uint64>(x)) !=
                    kVIA_OK)
                    return CCL_INTERNALERROR;
                break;
            default:
                return CCL_WRONGTYPE;
        }

        if (fbValue.mVIAObject->Commit() != kVIA_OK)
            return CCL_VALUEREADONLY;

        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::GetUnionSelector(int32_t *selector) const {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        if (mMemberType != VIAFbTypeTag::eUnion)
            return CCL_WRONGTYPE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue(GetMemberValue());
        if (fbValue.mVIAObject == nullptr)
            return CCL_VALUENOTACCESSIBLE;

        size_t index;
        if (fbValue.mVIAObject->GetValidMemberIndex(kVIAFbTypeMemberHandleWhole, &index) != kVIA_OK)
            return CCL_INTERNALERROR;

        *selector = static_cast<int32_t>(index);
        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::GetArraySize(int32_t *size) const {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        if (!IsMemberValid())
            return CCL_VALUENOTACCESSIBLE;

        if (mMemberType != VIAFbTypeTag::eArray)
            return CCL_WRONGTYPE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue(GetMemberValue());
        if (fbValue.mVIAObject == nullptr)
            return CCL_VALUENOTACCESSIBLE;

        size_t length;
        if (fbValue.mVIAObject->GetArrayLength(kVIAFbTypeMemberHandleWhole, &length) != kVIA_OK)
            return CCL_INTERNALERROR;

        *size = static_cast<int32_t>(length);
        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::SetArraySize(int32_t size) {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        if (mMemberType != VIAFbTypeTag::eArray)
            return CCL_WRONGTYPE;

        NDetail::VIAObjectGuard <VIAFbValue> fbValue(GetMemberValue());
        if (fbValue.mVIAObject == nullptr)
            return CCL_VALUENOTACCESSIBLE;

        NDetail::VIAObjectGuard <VIAFbType> fbType;
        if (fbValue.mVIAObject->GetType(&(fbType.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        VIAFbTypeTag typeTag;
        if (fbType.mVIAObject->GetTypeTag(&typeTag) != kVIA_OK)
            return CCL_INTERNALERROR;

        if (typeTag != VIAFbTypeTag::eArray)
            return CCL_WRONGTYPE;

        size_t length{static_cast<size_t>(size)};
        if (fbValue.mVIAObject->SetArrayLength(kVIAFbTypeMemberHandleWhole, length) != kVIA_OK)
            return CCL_ARRAYNOTRESIZABLE;

        if (fbValue.mVIAObject->Commit() != kVIA_OK)
            return CCL_VALUEREADONLY;

        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::GetValueType(cclValueType *valueType) const {
        switch (mMemberType) {
            case VIAFbTypeTag::eSignedInteger:
            case VIAFbTypeTag::eUnsignedInteger:
                *valueType = cclValueType::CCL_VALUE_TYPE_INTEGER;
                return CCL_SUCCESS;
            case VIAFbTypeTag::eFloat:
                *valueType = cclValueType::CCL_VALUE_TYPE_FLOAT;
                return CCL_SUCCESS;
            case VIAFbTypeTag::eString:
                *valueType = cclValueType::CCL_VALUE_TYPE_STRING;
                return CCL_SUCCESS;
            case VIAFbTypeTag::eArray:
                *valueType = cclValueType::CCL_VALUE_TYPE_ARRAY;
                return CCL_SUCCESS;
            case VIAFbTypeTag::eStruct:
                *valueType = cclValueType::CCL_VALUE_TYPE_STRUCT;
                return CCL_SUCCESS;
            case VIAFbTypeTag::eUnion:
                *valueType = cclValueType::CCL_VALUE_TYPE_UNION;
                return CCL_SUCCESS;
            default:
                break;
        }
        return CCL_UNSUPPORTEDTYPE;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::IsValid(bool *isValid) const {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        if (isValid == nullptr)
            return CCL_PARAMETERINVALID;

        NDetail::VIAObjectGuard <VIAFbValue> signalValue;
        if (this->mPort->GetSignalValue(mSignalName.c_str(), VIAFbTypeLevel::eImpl, &(signalValue.mVIAObject)) !=
            kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        if (signalValue.mVIAObject->IsValid(mMemberHandle, isValid) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::ClearValue() {
        if (this->mPort == nullptr)
            return CCL_INTERNALERROR;

        if (mMemberHandle == kVIAFbTypeMemberHandleWhole)
            return CCL_VALUENOTOPTIONAL;

        NDetail::VIAObjectGuard <VIAFbValue> signalValue;
        if (this->mPort->GetSignalValue(mSignalName.c_str(), VIAFbTypeLevel::eImpl, &(signalValue.mVIAObject)) !=
            kVIA_OK)
            return CCL_INTERNALERROR;

        if (signalValue.mVIAObject->SetIsValid(mMemberHandle, false) != kVIA_OK)
            return CCL_VALUENOTOPTIONAL;

        if (signalValue.mVIAObject->Commit() != kVIA_OK)
            return CCL_VALUEREADONLY;

        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::GetUpdateTimeNS(cclTime *time) const {
        if (mMemberStatus == nullptr)
            return CCL_INTERNALERROR;

        VIATime value;
        if (mMemberStatus->GetLastUpdateTimestamp(&value) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        *time = value;
        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::GetChangeTimeNS(cclTime *time) const {
        if (mMemberStatus == nullptr)
            return CCL_INTERNALERROR;

        VIATime value;
        if (mMemberStatus->GetLastChangeTimestamp(&value) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        *time = value;
        return CCL_SUCCESS;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::GetState(cclValueState *state) const {
        if (mMemberStatus == nullptr)
            return CCL_INTERNALERROR;

        VIAFbValueState valueState;
        if (mMemberStatus->GetValueState(&valueState) != kVIA_OK)
            return CCL_VALUENOTACCESSIBLE;

        switch (valueState) {
            case VIAFbValueState::eOffline:
                *state = cclValueState::CCL_VALUE_STATE_OFFLINE;
                return CCL_SUCCESS;
            case VIAFbValueState::eMeasured:
                *state = cclValueState::CCL_VALUE_STATE_MEASURED;
                return CCL_SUCCESS;
            default:
                break;
        }

        return CCL_INTERNALERROR;
    }

    template<typename PortType>
    int32_t VPDUSignalValue<PortType>::SetHandler(bool onUpdate, cclValueHandler handler) {
        if (mMemberStatus == nullptr)
            return CCL_INTERNALERROR;

        if (mCallbackHandle != nullptr) {
            auto rc = mMemberStatus->UnregisterObserver(mCallbackHandle);
            mCallbackHandle = nullptr;
            mCallbackFunction = nullptr;
            if (rc != kVIA_OK)
                return CCL_INTERNALERROR;
        }

        mCallbackFunction = handler;

        if (mCallbackFunction != nullptr) {
            if (mMemberStatus->RegisterObserver(this, onUpdate ? VIAFbUpdateMode::eNotifyOnUpdate
                                                               : VIAFbUpdateMode::eNotifyOnChange, &mCallbackHandle) !=
                kVIA_OK) {
                mCallbackFunction = nullptr;
                mCallbackHandle = nullptr;
                return CCL_INTERNALERROR;
            }
        }

        return CCL_SUCCESS;
    }

#pragma endregion

#pragma region VFunction Implementation

    // Static function utilities
    namespace VFunctionUtils {
        int32_t LookupFunction(cclFunctionID functionID, IFunctionPort *&function) {
            if (gModule == nullptr)
                return CCL_INTERNALERROR;

            function = gModule->mFunctions.Get(functionID);
            if (function == nullptr)
                return CCL_INVALIDFUNCTIONID;

            return CCL_SUCCESS;
        }

        int32_t MapCallState(cclCallState callState, VIAFbFunctionCallState &mapped) {
            switch (callState) {
                case cclCallState::CCL_CALL_STATE_CALLING:
                    mapped = VIAFbFunctionCallState::eCalling;
                    return 0;
                case cclCallState::CCL_CALL_STATE_CALLED:
                    mapped = VIAFbFunctionCallState::eCalled;
                    return 1;
                case cclCallState::CCL_CALL_STATE_RETURNING:
                    mapped = VIAFbFunctionCallState::eReturning;
                    return 2;
                case cclCallState::CCL_CALL_STATE_RETURNED:
                    mapped = VIAFbFunctionCallState::eReturned;
                    return 3;
                default:
                    break;
            }
            return -1;
        }

        int32_t MapCallState(VIAFbFunctionCallState callState, cclCallState &mapped) {
            switch (callState) {
                case VIAFbFunctionCallState::eCalling:
                    mapped = cclCallState::CCL_CALL_STATE_CALLING;
                    return 0;
                case VIAFbFunctionCallState::eCalled:
                    mapped = cclCallState::CCL_CALL_STATE_CALLED;
                    return 1;
                case VIAFbFunctionCallState::eReturning:
                    mapped = cclCallState::CCL_CALL_STATE_RETURNING;
                    return 2;
                case VIAFbFunctionCallState::eReturned:
                    mapped = cclCallState::CCL_CALL_STATE_RETURNED;
                    return 3;
                default:
                    break;
            }
            return -1;
        }

        cclCallContextID CreateCallContext(VIAFbCallContext *ctxt, cclFunctionID funcID, bool release) {
            if (gModule == nullptr)
                return CCL_INTERNALERROR;

            return gModule->mCallContexts.Add(new VCallContext(funcID, ctxt, release));
        }

        bool DestroyCallContext(cclCallContextID ctxtID) {
            if (gModule == nullptr)
                return false;

            return gModule->mCallContexts.Remove(ctxtID);
        }
    }

    // Consumer side function class
    VClientFunction::VClientFunction(VIAFbFunctionClientPort *port, const char *portPath)
            : VClientFunctionBase(port, CCL_SIDE_CONSUMER, portPath) {}

    VClientFunction::~VClientFunction() {}

    int32_t VClientFunction::CreateClientFunction(const char *portPath) {
        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbFunctionClientPort *port;
        if (fbAPI->GetFunctionClientPort(portPath, &port) != kVIA_OK)
            return CCL_INVALIDFUNCTIONPATH;

        auto fct = new VClientFunction(port, portPath);
        return gModule->mFunctions.Add(fct);
    }

    template<typename T, typename IF, typename Subclass>
    cclCallContextID VClientFunctionBase<T, IF, Subclass>::FindCallContext(VIAFbCallContext *ctxt) {
        int64_t requestID;
        if (ctxt->GetRequestID(&requestID) != kVIA_OK)
            return -1;

        // call contexts of synchronous calls receive a flag in their request ID
        // since the call context is put into the map with the original request ID (before it is known
        // whether a sync or async call is desired), search it without regard of this flag
        requestID &= ~VCallContext::kIsSynchronousCallFlag;

        auto it = mRequestIDMap.find(requestID);
        if (it != mRequestIDMap.end())
            return it->second;

        return NDetail::cInvalidCCLObjectID;
    }

    template<typename T, typename IF, typename Subclass>
    cclCallContextID VClientFunctionBase<T, IF, Subclass>::CreateCallContext(VIAFbCallContext *ctxt) {
        int64_t requestID;
        if (ctxt->GetRequestID(&requestID) != kVIA_OK) {
            ctxt->Release();
            return CCL_INTERNALERROR;
        }

        cclCallContextID ctxtID = VFunctionUtils::CreateCallContext(ctxt, this->mFunctionID, true);
        if (ctxtID == NDetail::cInvalidCCLObjectID) {
            ctxt->Release();
            return CCL_TOOMANYCALLCONTEXTS;
        }

        mRequestIDMap[requestID] = ctxtID;
        return ctxtID;
    }

    template<typename T, typename IF, typename Subclass>
    bool VClientFunctionBase<T, IF, Subclass>::DestroyCallContext(VIAFbCallContext *ctxt) {
        int64_t requestID;
        if (ctxt->GetRequestID(&requestID) != kVIA_OK)
            return false;

        auto rqit = mRequestIDMap.find(requestID);
        if (rqit == mRequestIDMap.end())
            return false;

        cclCallContextID ctxtID = rqit->second;
        mRequestIDMap.erase(rqit);

        return VFunctionUtils::DestroyCallContext(ctxtID);
    }

    template<typename T, typename IF, typename Subclass>
    VIAResult
    VClientFunctionBase<T, IF, Subclass>::HandleCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                                                 VIAFbFunctionCallState inCallState) {
        if (inCallState == VIAFbFunctionCallState::eFinalizing) {
            return DestroyCallContext(inCallContext) ? kVIA_OK : kVIA_Failed;
        }

        cclCallState cclState;
        int32_t callbackIndex = VFunctionUtils::MapCallState(inCallState, cclState);
        if ((callbackIndex < 0) || (callbackIndex > 3))
            return kVIA_Failed;

        if (this->mCallbacks[callbackIndex].mCallbackFunction != nullptr) {
            bool useNewCallContext = false;
            cclCallContextID ctxtID = FindCallContext(inCallContext);
            if (ctxtID == NDetail::cInvalidCCLObjectID) {
                // call may have been started through different APIs, create temporary context
                useNewCallContext = true;
            } else {
                VIAFbFunctionCallSide side;
                auto ret = inCallContext->GetSide(&side);
                if (ret != kVIA_OK) return ret;
                auto existingCC = gModule->mCallContexts.Get(ctxtID);
                if (!existingCC) {
                    useNewCallContext = true;
                } else if (existingCC->mSide != side) {
                    // happens with internal functions
                    useNewCallContext = true;
                }
            }
            if (useNewCallContext) {
                ctxtID = VFunctionUtils::CreateCallContext(inCallContext, this->mFunctionID, false);
                if (ctxtID == NDetail::cInvalidCCLObjectID)
                    return kVIA_Failed;

                this->mCallbacks[callbackIndex].mCallbackFunction(static_cast<cclTime>(inTime), ctxtID);

                if (!VFunctionUtils::DestroyCallContext(ctxtID))
                    return kVIA_Failed;
            } else {
                this->mCallbacks[callbackIndex].mCallbackFunction(static_cast<cclTime>(inTime), ctxtID);
            }
        }
        return kVIA_OK;
    }

    VIASTDDEF VClientFunction::OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                                  VIAFbFunctionCallState inCallState,
                                                  const VIAFbFunctionClientPort *inClientPort) {
        return HandleCallStateChanged(inTime, inCallContext, inCallState);
    }

    cclCallContextID VClientFunction::BeginCall() {
        VIAFbCallContext *ctxt;
        if (mPort->BeginCall(&ctxt) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CreateCallContext(ctxt);
    }

    int32_t VClientFunction::InvokeCall(VCallContext *callContext) {
        if (mPort->InvokeCall(callContext, callContext->mContext) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

    // Provider side function class
    VServerFunction::VServerFunction(VIAFbFunctionServerPort *port, const char *portPath)
            : VServerFunctionBase(port, CCL_SIDE_PROVIDER, portPath) {}

    VServerFunction::~VServerFunction() {}

    int32_t VServerFunction::CreateServerFunction(const char *portPath) {
        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbFunctionServerPort *port;
        if (fbAPI->GetFunctionServerPort(portPath, &port) != kVIA_OK)
            return CCL_INVALIDFUNCTIONPATH;

        auto fct = new VServerFunction(port, portPath);
        return gModule->mFunctions.Add(fct);
    }

    template<typename T, typename IF, typename Subclass>
    VIAResult
    VServerFunctionBase<T, IF, Subclass>::HandleCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                                                 VIAFbFunctionCallState inCallState) {
        cclCallState cclState;
        int32_t callbackIndex = VFunctionUtils::MapCallState(inCallState, cclState);
        if ((callbackIndex < 0) || (callbackIndex > 3))
            return kVIA_Failed;

        if (this->mCallbacks[callbackIndex].mCallbackFunction == nullptr)
            return kVIA_OK;

        // call may have been started through different APIs, create temporary context
        auto ctxtID = VFunctionUtils::CreateCallContext(inCallContext, this->mFunctionID, false);
        if (ctxtID == NDetail::cInvalidCCLObjectID)
            return kVIA_Failed;

        this->mCallbacks[callbackIndex].mCallbackFunction(static_cast<cclTime>(inTime), ctxtID);
        return VFunctionUtils::DestroyCallContext(ctxtID) ? kVIA_OK : kVIA_Failed;
    }

    VIASTDDEF VServerFunction::OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                                  VIAFbFunctionCallState inCallState,
                                                  const VIAFbFunctionServerPort *inServerPort) {
        return HandleCallStateChanged(inTime, inCallContext, inCallState);
    }

    // Method base class

    VMethodBase::~VMethodBase() {
        if (mMethod != nullptr) {
            mMethod->Release();
        }
    }

    VIAResult
    VMethodBase::DoRegisterCallStateObserver(VIAFbDOMethodObserver *pObserver, VIAFbFunctionCallState callState,
                                             VIAFbCallbackHandle *handle) {
        if (mMember != nullptr) {
            if (mMethod == nullptr) {
                auto result = VIAQueryFbDOMemberInterface(mMember, &mMethod);
                if (result != kVIA_OK)
                    return result;
            }
            return mMethod->RegisterObserver(pObserver, callState, handle);
        } else {
            return kVIA_ObjectInvalid;
        }
    }

    void VMethodBase::DoUnregisterCallStateObserver(VIAFbCallbackHandle handle) {
        if (mMethod != nullptr) {
            mMethod->UnregisterObserver(handle);
        }
    }

    // Consumed method class
    VConsumedMethod::VConsumedMethod(VIAFbDOMember *doMethod, const char *doMemberPath)
            : VClientFunctionBase(doMethod, CCL_SIDE_CONSUMER, doMemberPath) {
    }

    VConsumedMethod::~VConsumedMethod() {
    }

    VIAResult VConsumedMethod::OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                                  VIAFbFunctionCallState inCallState, const VIAFbDOMethod *inMethod) {
        return HandleCallStateChanged(inTime, inCallContext, inCallState);
    }

    cclCallContextID VConsumedMethod::BeginCall() {
        VIAFbDOMethodConsumer *pConsumer = nullptr;
        auto result = VIAQueryFbDOMemberInterface(mMember, &pConsumer);
        if (result != kVIA_OK)
            return CCL_INTERNALERROR;

        VIAFbCallContext *ctxt;
        result = pConsumer->BeginCall(&ctxt);
        pConsumer->Release();
        if (result != kVIA_OK)
            return CCL_INTERNALERROR;

        return CreateCallContext(ctxt);
    }

    int32_t VConsumedMethod::InvokeCall(VCallContext *callContext) {
        VIAFbDOMethodConsumer *pConsumer = nullptr;
        auto result = VIAQueryFbDOMemberInterface(mMember, &pConsumer);
        if (result != kVIA_OK)
            return CCL_INTERNALERROR;

        result = pConsumer->InvokeCall(callContext, callContext->mContext);
        if (result != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

    int32_t VConsumedMethod::InvokeSynchronousCall(VCallContext *callContext) {
        VIAFbDOMethodConsumer *pConsumer = nullptr;
        auto result = VIAQueryFbDOMemberInterface(mMember, &pConsumer);
        if (result != kVIA_OK)
            return CCL_INTERNALERROR;

        result = pConsumer->InvokeSynchronousCall(callContext, callContext->mContext);
        if (result != kVIA_OK)
            return CCL_NO_SYNCHRONOUS_CALL;

        return CCL_SUCCESS;
    }

    // Provided method class
    VProvidedMethod::VProvidedMethod(VIAFbDOMember *doMethod, const char *doMemberPath)
            : VServerFunctionBase(doMethod, CCL_SIDE_PROVIDER, doMemberPath) {
    }

    VProvidedMethod::~VProvidedMethod() {
    }

    VIAResult VProvidedMethod::OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                                  VIAFbFunctionCallState inCallState, const VIAFbDOMethod *inMethod) {
        return HandleCallStateChanged(inTime, inCallContext, inCallState);
    }

    int32_t DOMethods::CreateDOMethod(const char *doMemberPath) {
        VIAFbDOMember *pMember = nullptr;
        auto result = VDOMemberBase::GetDOMemberByPath(doMemberPath, &pMember);
        if (result != CCL_SUCCESS)
            return result;

        VIAFbDOMemberDirection direction;
        auto viaResult = pMember->GetDirection(&direction);
        if (viaResult != kVIA_OK) {
            pMember->Release();
            return CCL_INTERNALERROR;
        }

        switch (direction) {
            case eFbDOMemberDirection_Consumed:
            case eFbDOMemberDirection_Internal: {
                VIAFbDOMethodConsumer *pConsumer = nullptr;
                auto result = VIAQueryFbDOMemberInterface(pMember, &pConsumer);
                if (result != kVIA_OK) {
                    pMember->Release();
                    return CCL_INTERNALERROR;
                }
                pConsumer->Release();
                auto method = new VConsumedMethod(pMember, doMemberPath);
                return gModule->mFunctions.Add(method);
            }
            case eFbDOMemberDirection_Provided: {
                VIAFbDOMethodProvider *pProvider = nullptr;
                auto result = VIAQueryFbDOMemberInterface(pMember, &pProvider);
                if (result != kVIA_OK) {
                    pMember->Release();
                    return CCL_INTERNALERROR;
                }
                pProvider->Release();
                auto method = new VProvidedMethod(pMember, doMemberPath);
                return gModule->mFunctions.Add(method);
            }
            default:
                pMember->Release();
                return CCL_INTERNALERROR;
        }
    }

    // Call context class
    VCallContext::VCallContext(cclFunctionID funcID, VIAFbCallContext *ctxt, bool release)
            : mFunctionID(funcID), mContext(ctxt), mRequestID(-1LL), mSide(VIAFbFunctionCallSide::eClient),
              mRelease(release) {
        mContext->GetRequestID(&mRequestID);
        mContext->GetSide(&mSide);
    }

    VCallContext::~VCallContext() {
        if (mRelease && (mContext != nullptr)) mContext->Release();

        for (auto valueID: mContextValues) {
            gModule->mValues.Remove(valueID);
        }
    }

    VIASTDDEF VCallContext::OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                               VIAFbFunctionCallState inCallState,
                                               const VIAFbFunctionClientPort *inClientPort) {
        if ((mResultCallbackFunction != nullptr) && (inCallState == VIAFbFunctionCallState::eReturned))
            mResultCallbackFunction(static_cast<cclTime>(inTime), mContextID);

        return kVIA_OK;
    }

    VIASTDDEF VCallContext::OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                               VIAFbFunctionCallState inCallState, const VIAFbDOMethod *inMethod) {
        if ((mResultCallbackFunction != nullptr) && (inCallState == VIAFbFunctionCallState::eReturned))
            mResultCallbackFunction(static_cast<cclTime>(inTime), mContextID);

        return kVIA_OK;
    }

    bool VCallContext::IsSynchronousCall() const {
        int64 requestID;
        auto ret = mContext->GetRequestID(&requestID);
        if (ret != kVIA_OK)
            return false;
        return (requestID & kIsSynchronousCallFlag) != 0;
    }

#pragma endregion

#pragma region VService Implementation

    VService::VService()
            : mServiceID(-1), mService(nullptr), mServiceDiscoveryHandler(nullptr), mConsumerDiscoveredHandler(nullptr),
              mProviderDiscoveredHandler(nullptr), mCallbackHandle(nullptr) {}

    VService::~VService() {
        if (mCallbackHandle != nullptr) {
            if (mService != nullptr) mService->UnregisterObserver(mCallbackHandle);
            mServiceDiscoveryHandler = nullptr;
            mConsumerDiscoveredHandler = nullptr;
            mProviderDiscoveredHandler = nullptr;
            mCallbackHandle = nullptr;
        }
        if (mService != nullptr) mService->Release();
    }

    int32_t VService::CreateService(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        cclServiceID serviceID = static_cast<cclServiceID>(gModule->mServices.size());
        std::unique_ptr<VService> service{new VService()};
        service->mServiceID = serviceID;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        if (fbAPI->GetServiceCO(path, &(service->mService)) != kVIA_OK)
            return CCL_INVALIDSERVICEPATH;

        gModule->mServices.push_back(service.release());

        return serviceID;
    }

    int32_t VService::LookupService(cclServiceID serviceID, VService *&service) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        if (serviceID < 0 || serviceID >= static_cast<int32_t>(gModule->mServices.size()))
            return CCL_INVALIDSERVICEID;

        service = gModule->mServices[serviceID];
        if ((service == nullptr) || (service->mServiceID != serviceID))
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

    void VService::Cleanup() {
        for (auto *service: gModule->mServices) delete service;
        gModule->mServices.clear();
    }

    int32_t VService::UpdateRegistration() {
        if (mService == nullptr)
            return CCL_INTERNALERROR;

        VIAResult rv = kVIA_OK;
        if ((mServiceDiscoveryHandler != nullptr) || (mConsumerDiscoveredHandler != nullptr) ||
            (mProviderDiscoveredHandler != nullptr)) {
            if (mCallbackHandle == nullptr)
                rv = mService->RegisterObserver(this, &mCallbackHandle);
        } else {
            if (mCallbackHandle != nullptr)
                rv = mService->UnregisterObserver(mCallbackHandle);
            mCallbackHandle = nullptr;
        }
        return (rv == kVIA_OK) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

    int32_t VService::SetServiceDiscoveryHandler(cclServiceDiscoveryHandler handler) {
        mServiceDiscoveryHandler = handler;
        return UpdateRegistration();
    }

    int32_t VService::SetConsumerDiscoveredHandler(cclConsumerDiscoveredHandler handler) {
        mConsumerDiscoveredHandler = handler;
        return UpdateRegistration();
    }

    int32_t VService::SetProviderDiscoveredHandler(cclProviderDiscoveredHandler handler) {
        mProviderDiscoveredHandler = handler;
        return UpdateRegistration();
    }

    VIASTDDEF VService::OnServiceDiscovery(VIAFbServiceCO *inService, VIAFbServiceConsumer *inConsumer) {
        if (mService != inService)
            return kVIA_Failed;

        if (mServiceDiscoveryHandler != nullptr) {
            cclConsumerID consumerID = VConsumer::CreateConsumer(inConsumer, false);
            if (consumerID < 0)
                return kVIA_Failed;

            mServiceDiscoveryHandler(consumerID);
            return VConsumer::RemoveConsumer(consumerID);
        }
        return kVIA_OK;
    }

    VIASTDDEF VService::OnConsumerDiscovered(VIAFbServiceCO *inService, VIAFbAddressHandle *inAddress) {
        if (mService != inService)
            return kVIA_Failed;

        if (mConsumerDiscoveredHandler != nullptr) {
            auto addressID = VAddress::CreateAddress(inAddress, false);
            if (addressID < 0)
                return kVIA_Failed;

            mConsumerDiscoveredHandler(addressID);
            return VAddress::RemoveAddress(addressID);
        }
        return kVIA_OK;
    }

    VIASTDDEF VService::OnProviderDiscovered(VIAFbServiceCO *inService, VIAFbAddressHandle *inAddress) {
        if (mService != inService)
            return kVIA_Failed;

        if (mProviderDiscoveredHandler != nullptr) {
            auto addressID = VAddress::CreateAddress(inAddress, false);
            if (addressID < 0)
                return kVIA_Failed;

            mProviderDiscoveredHandler(addressID);
            return VAddress::RemoveAddress(addressID);
        }
        return kVIA_OK;
    }

#pragma endregion

#pragma region VConsumer Implementation

    VConsumer::VConsumer(VIAFbServiceConsumer *consumer, bool release)
            : VPortBase(consumer, release), mConsumerID(NDetail::cInvalidCCLObjectID),
              mProviderDiscoveredHandler(nullptr), mCallbackHandle(nullptr) {}

    VConsumer::~VConsumer() {
        if (mCallbackHandle != nullptr) {
            if (mPort != nullptr)
                mPort->UnregisterObserver(mCallbackHandle);

            mProviderDiscoveredHandler = nullptr;
            mCallbackHandle = nullptr;
        }
    }

    int32_t VConsumer::CreateConsumer(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbServiceConsumer *consumer = nullptr;
        if (fbAPI->GetServiceConsumer(path, &consumer) != kVIA_OK)
            return CCL_INVALIDCONSUMERPATH;

        return gModule->mConsumers.Add(new VConsumer(consumer, true));
    }

    int32_t VConsumer::CreateConsumer(VIAFbServiceConsumer *consumer, bool release) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mConsumers.Add(new VConsumer(consumer, release));
    }

    int32_t VConsumer::LookupConsumer(cclConsumerID consumerID, VConsumer *&consumer) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        consumer = gModule->mConsumers.Get(consumerID);
        if (consumer == nullptr)
            return CCL_INVALIDCONSUMERID;

        return CCL_SUCCESS;
    }

    int32_t VConsumer::RemoveConsumer(cclConsumerID consumerID) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VConsumer *consumer = gModule->mConsumers.Get(consumerID);
        if (consumer == nullptr)
            return CCL_INVALIDCONSUMERID;

        return gModule->mConsumers.Remove(consumerID) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

    int32_t VConsumer::SetProviderDiscoveredHandler(cclProviderDiscoveredHandler handler) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        mProviderDiscoveredHandler = handler;

        VIAResult rv = kVIA_OK;
        if (mProviderDiscoveredHandler != nullptr) {
            if (mCallbackHandle == nullptr)
                rv = mPort->RegisterObserver(this, &mCallbackHandle);
        } else {
            if (mCallbackHandle != nullptr)
                rv = mPort->UnregisterObserver(mCallbackHandle);
            mCallbackHandle = nullptr;
        }
        return (rv == kVIA_OK) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

    VIASTDDEF VConsumer::OnProviderDiscovered(VIAFbServiceConsumer *inConsumer, VIAFbAddressHandle *inAddress) {
        if (mProviderDiscoveredHandler != nullptr) {
            auto addressID = VAddress::CreateAddress(inAddress, false);
            if (addressID < 0)
                return kVIA_Failed;

            mProviderDiscoveredHandler(addressID);
            return VAddress::RemoveAddress(addressID);
        }
        return kVIA_OK;
    }

#pragma endregion

#pragma region VProvider Implementation

    VProvider::VProvider(VIAFbServiceProvider *provider, bool release)
            : VPortBase(provider, release), mProviderID(NDetail::cInvalidCCLObjectID),
              mConnectionRequestedHandler(nullptr), mConsumerDiscoveredHandler(nullptr), mCallbackHandle(nullptr) {}

    VProvider::~VProvider() {
        if (mCallbackHandle != nullptr) {
            if (mPort != nullptr)
                mPort->UnregisterObserver(mCallbackHandle);

            mConnectionRequestedHandler = nullptr;
            mConsumerDiscoveredHandler = nullptr;
            mCallbackHandle = nullptr;
        }
    }

    int32_t VProvider::CreateProvider(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbServiceProvider *provider = nullptr;
        if (fbAPI->GetServiceProvider(path, &provider) != kVIA_OK)
            return CCL_INVALIDPROVIDERPATH;

        return gModule->mProviders.Add(new VProvider(provider, true));
    }

    int32_t VProvider::CreateProvider(VIAFbServiceProvider *provider, bool release) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mProviders.Add(new VProvider(provider, release));
    }

    int32_t VProvider::LookupProvider(cclProviderID providerID, VProvider *&provider) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        provider = gModule->mProviders.Get(providerID);
        if (provider == nullptr)
            return CCL_INVALIDPROVIDERID;

        return CCL_SUCCESS;
    }

    int32_t VProvider::RemoveProvider(cclProviderID providerID) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VProvider *provider = gModule->mProviders.Get(providerID);
        if (provider == nullptr)
            return CCL_INVALIDPROVIDERID;

        return gModule->mProviders.Remove(providerID) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

    int32_t VProvider::UpdateRegistration() {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        VIAResult rv = kVIA_OK;
        if ((mConnectionRequestedHandler != nullptr) || (mConsumerDiscoveredHandler != nullptr)) {
            if (mCallbackHandle == nullptr)
                rv = mPort->RegisterObserver(this, &mCallbackHandle);
        } else {
            if (mCallbackHandle != nullptr)
                rv = mPort->UnregisterObserver(mCallbackHandle);
            mCallbackHandle = nullptr;
        }
        return (rv == kVIA_OK) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

    int32_t VProvider::SetConnectionRequestedHandler(cclConnectionRequestedHandler handler) {
        mConnectionRequestedHandler = handler;
        return UpdateRegistration();
    }

    int32_t VProvider::SetConsumerDiscoveredHandler(cclConsumerDiscoveredHandler handler) {
        mConsumerDiscoveredHandler = handler;
        return UpdateRegistration();
    }

    VIASTDDEF VProvider::OnConsumerDiscovered(VIAFbServiceProvider *inProvider, VIAFbAddressHandle *inAddress) {
        if (mConsumerDiscoveredHandler != nullptr) {
            auto addressID = VAddress::CreateAddress(inAddress, false);
            if (addressID < 0)
                return kVIA_Failed;

            mConsumerDiscoveredHandler(addressID);
            return VAddress::RemoveAddress(addressID);
        }
        return kVIA_OK;
    }

    VIASTDDEF VProvider::OnConnectionRequested(VIAFbServiceProvider *inProvider, VIAFbServiceConsumer *inConsumer) {
        if (mConnectionRequestedHandler != nullptr) {
            cclConsumerID consumerID = VConsumer::CreateConsumer(inConsumer, false);
            if (consumerID < 0)
                return kVIA_Failed;

            mConnectionRequestedHandler(consumerID);
            return VConsumer::RemoveConsumer(consumerID);
        }
        return kVIA_OK;
    }

#pragma endregion

#pragma region VConsumedService Implementation

    VConsumedService::VConsumedService(VIAFbConsumedService *consumedService)
            : VPortBase(consumedService, true), mConsumedServiceID(NDetail::cInvalidCCLObjectID) {}

    VConsumedService::~VConsumedService() {
        for (auto *handler: mConnectionHandlerPool) delete handler;
        for (auto *handler: mActiveConnectionHandlers) delete handler;
    }

    int32_t VConsumedService::CreateConsumedService(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbConsumedService *consumedService = nullptr;
        if (fbAPI->GetConsumedService(path, &consumedService) != kVIA_OK)
            return CCL_INVALIDCONSUMEDSERVICEPATH;

        return gModule->mConsumedServices.Add(new VConsumedService(consumedService));
    }

    int32_t VConsumedService::CreateConsumedService(VIAFbConsumedService *consumedService) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mConsumedServices.Add(new VConsumedService(consumedService));
    }

    int32_t VConsumedService::LookupConsumedService(cclConsumedServiceID consumedServiceID,
                                                    VConsumedService *&consumedService) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        consumedService = gModule->mConsumedServices.Get(consumedServiceID);
        if (consumedService == nullptr)
            return CCL_INVALIDCONSUMEDSERVICEID;

        return CCL_SUCCESS;
    }

    int32_t VConsumedService::ConnectAsync(cclConnectionEstablishedHandler successCallback,
                                           cclConnectionFailedHandler failureCallback) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        if (mConnectionHandlerPool.empty())
            mConnectionHandlerPool.push_back(new ConnectionHandler());

        ConnectionHandler *handler = mConnectionHandlerPool.back();

        handler->mParent = this;
        handler->mSuccessCallback = successCallback;
        handler->mFailureCallback = failureCallback;

        mConnectionHandlerPool.pop_back();
        mActiveConnectionHandlers.insert(handler);

        if (mPort->ConnectAsync(handler) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

    VIASTDDEF VConsumedService::ConnectionHandler::OnConnectionEstablished(VIAFbConsumedService *inPort) {
        if (mSuccessCallback != nullptr) mSuccessCallback();
        mSuccessCallback = nullptr;
        mFailureCallback = nullptr;

        if (mParent != nullptr) {
            mParent->mActiveConnectionHandlers.erase(this);
            mParent->mConnectionHandlerPool.push_back(this);
            mParent = nullptr;
        }
        return kVIA_OK;
    }

    VIASTDDEF VConsumedService::ConnectionHandler::OnConnectionFailed(VIAFbConsumedService *inPort,
                                                                      const char *inError) {
        if (mFailureCallback != nullptr) mFailureCallback(inError);
        mSuccessCallback = nullptr;
        mFailureCallback = nullptr;

        if (mParent != nullptr) {
            mParent->mActiveConnectionHandlers.erase(this);
            mParent->mConnectionHandlerPool.push_back(this);
            mParent = nullptr;
        }
        return kVIA_OK;
    }

#pragma endregion

#pragma region VProvidedService Implementation

    VProvidedService::VProvidedService(VIAFbProvidedService *providedService)
            : VPortBase(providedService, true), mProvidedServiceID(NDetail::cInvalidCCLObjectID),
              mSimulatorCallback(nullptr), mSimulatorHandle(nullptr) {}

    VProvidedService::~VProvidedService() {
        if (mSimulatorHandle != nullptr) {
            if (mPort != nullptr)
                mPort->UnregisterSimulator(mSimulatorHandle);

            mSimulatorHandle = nullptr;
            mSimulatorCallback = nullptr;
        }

        for (auto *handler: mConnectionHandlerPool) delete handler;
        for (auto *handler: mActiveConnectionHandlers) delete handler;
    }

    int32_t VProvidedService::CreateProvidedService(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbProvidedService *providedService = nullptr;
        if (fbAPI->GetProvidedService(path, &providedService) != kVIA_OK)
            return CCL_INVALIDPROVIDEDSERVICEPATH;

        return gModule->mProvidedServices.Add(new VProvidedService(providedService));
    }

    int32_t VProvidedService::CreateProvidedService(VIAFbProvidedService *providedService) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mProvidedServices.Add(new VProvidedService(providedService));
    }

    int32_t VProvidedService::LookupProvidedService(cclProvidedServiceID providedServiceID,
                                                    VProvidedService *&providedService) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        providedService = gModule->mProvidedServices.Get(providedServiceID);
        if (providedService == nullptr)
            return CCL_INVALIDPROVIDEDSERVICEID;

        return CCL_SUCCESS;
    }

    int32_t VProvidedService::SetSimulator(cclCallHandler simulator) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        if (mSimulatorHandle != nullptr) {
            auto rc = mPort->UnregisterSimulator(mSimulatorHandle);
            mSimulatorHandle = nullptr;
            mSimulatorCallback = nullptr;
            if (rc != kVIA_OK)
                return CCL_INTERNALERROR;
        }

        mSimulatorCallback = simulator;

        if (mSimulatorCallback != nullptr) {
            if (mPort->RegisterSimulator(this, &mSimulatorHandle) != kVIA_OK) {
                mSimulatorHandle = nullptr;
                mSimulatorCallback = nullptr;
                return CCL_INTERNALERROR;
            }
        }

        return CCL_SUCCESS;
    }

    cclFunctionID VProvidedService::FindFunction(VIAFbCallContext *callContext) {
        static char buffer[4096];
        if (callContext->GetServerPort(buffer, 4096) != kVIA_OK)
            return CCL_INTERNALERROR;

        std::string serverPort(buffer);
        auto it = mFunctionMap.find(serverPort);
        if (it != mFunctionMap.end()) return it->second;

        // first call state notification for this function, create new function
        cclFunctionID functionID = VServerFunction::CreateServerFunction(serverPort.c_str());
        if (functionID >= 0) mFunctionMap[serverPort] = functionID;
        return functionID;
    }

    VIASTDDEF VProvidedService::OnCallStateChanged(VIATime inTime, VIAFbCallContext *inCallContext,
                                                   VIAFbFunctionCallState inCallState,
                                                   const VIAFbFunctionServerPort *inServerPort) {
        if (inCallState != VIAFbFunctionCallState::eCalled)
            return kVIA_OK;

        if (mSimulatorCallback == nullptr)
            return kVIA_OK;

        auto funcID = FindFunction(inCallContext);
        if (funcID == NDetail::cInvalidCCLObjectID)
            return kVIA_Failed;

        auto ctxtID = VFunctionUtils::CreateCallContext(inCallContext, funcID, false);
        if (ctxtID == NDetail::cInvalidCCLObjectID)
            return kVIA_Failed;

        mSimulatorCallback(static_cast<cclTime>(inTime), ctxtID);

        return VFunctionUtils::DestroyCallContext(ctxtID) ? kVIA_OK : kVIA_Failed;
    }

    int32_t VProvidedService::ConnectAsync(cclConnectionEstablishedHandler successCallback,
                                           cclConnectionFailedHandler failureCallback) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        if (mConnectionHandlerPool.empty())
            mConnectionHandlerPool.push_back(new ConnectionHandler());

        ConnectionHandler *handler = mConnectionHandlerPool.back();

        handler->mParent = this;
        handler->mSuccessCallback = successCallback;
        handler->mFailureCallback = failureCallback;

        mConnectionHandlerPool.pop_back();
        mActiveConnectionHandlers.insert(handler);

        if (mPort->ConnectAsync(handler) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

    VIASTDDEF VProvidedService::ConnectionHandler::OnConnectionEstablished(VIAFbProvidedService *inPort) {
        if (mSuccessCallback != nullptr) mSuccessCallback();
        mSuccessCallback = nullptr;
        mFailureCallback = nullptr;

        if (mParent != nullptr) {
            mParent->mActiveConnectionHandlers.erase(this);
            mParent->mConnectionHandlerPool.push_back(this);
            mParent = nullptr;
        }
        return kVIA_OK;
    }

    VIASTDDEF VProvidedService::ConnectionHandler::OnConnectionFailed(VIAFbProvidedService *inPort,
                                                                      const char *inError) {
        if (mFailureCallback != nullptr) mFailureCallback(inError);
        mSuccessCallback = nullptr;
        mFailureCallback = nullptr;

        if (mParent != nullptr) {
            mParent->mActiveConnectionHandlers.erase(this);
            mParent->mConnectionHandlerPool.push_back(this);
            mParent = nullptr;
        }
        return kVIA_OK;
    }

#pragma endregion

#pragma region VAddress Implementation

    VAddress::VAddress(VIAFbAddressHandle *address, bool release)
            : mAddressID(NDetail::cInvalidCCLObjectID), mAddress(address), mRelease(release) {}

    VAddress::~VAddress() {
        if (mAddress && mRelease) mAddress->Release();
    }

    cclAddressID VAddress::CreateAddress(VIAFbAddressHandle *address, bool release) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mAddresses.Add(new VAddress(address, release));
    }

    int32_t VAddress::LookupAddress(cclAddressID addressID, VAddress *&address) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        address = gModule->mAddresses.Get(addressID);
        if (address == nullptr)
            return CCL_INVALIDADDRESSID;

        return CCL_SUCCESS;
    }

    int32_t VAddress::RemoveAddress(cclAddressID addressID) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VAddress *address = gModule->mAddresses.Get(addressID);
        if (address == nullptr)
            return CCL_INVALIDADDRESSID;

        return gModule->mAddresses.Remove(addressID) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

#pragma endregion

#pragma region VPDUSender Implementation

    // base class for generic PDU Sender Port functionality
    VPDUSender::VPDUSender(VIAFbPDUSenderPort *pduSender, bool release)
            : VPortBase(pduSender, release), mPDUSenderID(NDetail::cInvalidCCLObjectID) {}

    VPDUSender::~VPDUSender() {}

    int32_t VPDUSender::CreatePDUSender(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbPDUSenderPort *pduSender = nullptr;
        if (fbAPI->GetPDUSenderPort(path, &pduSender) != kVIA_OK)
            return CCL_INVALIDPDUSENDERPATH;

        return gModule->mPDUSender.Add(new VPDUSender(pduSender, true));
    }

    int32_t VPDUSender::LookupPDUSender(cclPDUSenderID pduSenderID, VPDUSender *&pduSender) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        pduSender = gModule->mPDUSender.Get(pduSenderID);
        if (pduSender == nullptr)
            return CCL_INVALIDPDUSENDERID;

        return CCL_SUCCESS;
    }

#pragma endregion

#pragma region VPDUReceiver Implementation

    // base class for generic PDU Receiver Port functionality
    VPDUReceiver::VPDUReceiver(VIAFbPDUReceiverPort *pduReceiver)
            : VPortBase(pduReceiver, true), mPDUReceiverID(NDetail::cInvalidCCLObjectID) {}

    VPDUReceiver::~VPDUReceiver() {}

    int32_t VPDUReceiver::CreatePDUReceiver(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbPDUReceiverPort *pduReceiver = nullptr;
        if (fbAPI->GetPDUReceiverPort(path, &pduReceiver) != kVIA_OK)
            return CCL_INVALIDPDURECEIVERPATH;

        return gModule->mPDUReceiver.Add(new VPDUReceiver(pduReceiver));
    }

    int32_t VPDUReceiver::LookupPDUReceiver(cclPDUReceiverID pduReceiverID, VPDUReceiver *&pduReceiver) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        pduReceiver = gModule->mPDUReceiver.Get(pduReceiverID);
        if (pduReceiver == nullptr)
            return CCL_INVALIDPDURECEIVERID;

        return CCL_SUCCESS;
    }

#pragma endregion

#pragma region VConsumedServicePDU Implementation

    // base class for generic PDU functionality
    VConsumedServicePDU::VConsumedServicePDU(VIAFbConsumedServicePDU *consumedPDU)
            : VPDUReceiver(consumedPDU), mConsumedPDU(consumedPDU) {}

    VConsumedServicePDU::~VConsumedServicePDU() {}

    int32_t VConsumedServicePDU::CreateConsumedPDU(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbConsumedServicePDU *consumedPDU = nullptr;
        if (fbAPI->GetConsumedServicePDU(path, &consumedPDU) != kVIA_OK)
            return CCL_INVALIDCONSUMEDPDUPATH;

        return gModule->mPDUReceiver.Add(new VConsumedServicePDU(consumedPDU));
    }

    int32_t VConsumedServicePDU::CreateConsumedPDU(VIAFbConsumedServicePDU *consumedPDU) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mPDUReceiver.Add(new VConsumedServicePDU(consumedPDU));
    }

    int32_t VConsumedServicePDU::LookupConsumedPDU(cclPDUReceiverID consumedPDUID, VConsumedServicePDU *&consumedPDU) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        auto *pduReceiver = gModule->mPDUReceiver.Get(consumedPDUID);
        if (pduReceiver == nullptr)
            return CCL_INVALIDCONSUMEDPDUID;

        consumedPDU = dynamic_cast<VConsumedServicePDU *>(pduReceiver);
        if (consumedPDU == nullptr)
            return CCL_INVALIDCONSUMEDPDUID;

        return CCL_SUCCESS;
    }

    int32_t VConsumedServicePDU::SetSubscriptionStateIsolated(bool subscribed) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        auto state = subscribed ? VIAFbSubscriptionState::eSubscribed : VIAFbSubscriptionState::eSubscribable;
        if (mConsumedPDU->SetSubscriptionStateIsolated(state) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

    // abstract binding specific extensions
    VAbstractConsumedServicePDU::VAbstractConsumedServicePDU(VIAFbAbstractConsumedServicePDU *abstractConsumedPDU)
            : VConsumedServicePDU(abstractConsumedPDU), mAbstractConsumedPDU(abstractConsumedPDU) {}

    VAbstractConsumedServicePDU::~VAbstractConsumedServicePDU() {}

    int32_t VAbstractConsumedServicePDU::CreateConsumedPDU(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbAbstractBinding *abstractBinding;
        if (fbAPI->GetAbstractBinding(&abstractBinding) != kVIA_OK)
            return CCL_INTERNALERROR;

        VIAFbAbstractConsumedServicePDU *consumedPDU = nullptr;
        if (abstractBinding->GetConsumedServicePDU(path, &consumedPDU) != kVIA_OK)
            return CCL_INVALIDCONSUMEDPDUPATH;

        return gModule->mPDUReceiver.Add(new VAbstractConsumedServicePDU(consumedPDU));
    }

    int32_t VAbstractConsumedServicePDU::LookupConsumedPDU(cclPDUReceiverID consumedPDUID,
                                                           VAbstractConsumedServicePDU *&consumedPDU) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VPDUReceiver *pduReceiver = gModule->mPDUReceiver.Get(consumedPDUID);
        if (pduReceiver == nullptr)
            return CCL_INVALIDCONSUMEDPDUID;

        consumedPDU = dynamic_cast<VAbstractConsumedServicePDU *>(pduReceiver);
        if (consumedPDU == nullptr)
            return CCL_WRONGBINDING;

        return CCL_SUCCESS;
    }

#pragma endregion

#pragma region VProvidedServicePDU Implementation

    VProvidedServicePDU::VProvidedServicePDU(VIAFbProvidedServicePDU *providedPDU, bool release)
            : VPDUSender(providedPDU, release), mProvidedPDU(providedPDU) {}

    VProvidedServicePDU::~VProvidedServicePDU() {}

    int32_t VProvidedServicePDU::CreateProvidedPDU(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbProvidedServicePDU *providedPDU = nullptr;
        if (fbAPI->GetProvidedServicePDU(path, &providedPDU) != kVIA_OK)
            return CCL_INVALIDPROVIDEDPDUPATH;

        return gModule->mPDUSender.Add(new VProvidedServicePDU(providedPDU, true));
    }

    int32_t VProvidedServicePDU::CreateProvidedPDU(VIAFbProvidedServicePDU *providedPDU, bool release) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mPDUSender.Add(new VProvidedServicePDU(providedPDU, release));
    }

    int32_t VProvidedServicePDU::LookupProvidedPDU(cclPDUSenderID providedPDUID, VProvidedServicePDU *&providedPDU) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        auto *pduSender = gModule->mPDUSender.Get(providedPDUID);
        if (pduSender == nullptr)
            return CCL_INVALIDPROVIDEDPDUID;

        providedPDU = dynamic_cast<VProvidedServicePDU *>(pduSender);
        if (providedPDU == nullptr)
            return CCL_INVALIDPROVIDEDPDUID;

        return CCL_SUCCESS;
    }

    int32_t VProvidedServicePDU::RemoveProvidedPDU(cclPDUSenderID providedPDUID) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VPDUSender *providedPDU = gModule->mPDUSender.Get(providedPDUID);
        if (providedPDU == nullptr)
            return CCL_INVALIDPROVIDEDPDUID;

        return gModule->mPDUSender.Remove(providedPDUID) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

    int32_t VProvidedServicePDU::SetSubscriptionStateIsolated(bool subscribed) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        auto state = subscribed ? VIAFbSubscriptionState::eSubscribed : VIAFbSubscriptionState::eSubscribable;
        if (mProvidedPDU->SetSubscriptionStateIsolated(state) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

#pragma endregion

#pragma region VServicePDUProvider Implementation

    // base class for generic PDU functionality
    VServicePDUProvider::VServicePDUProvider(VIAFbServicePDUProvider *pduProvider)
            : VPortBase(pduProvider, true), mPDUProviderID(NDetail::cInvalidCCLObjectID) {}

    VServicePDUProvider::~VServicePDUProvider() {}

    int32_t VServicePDUProvider::CreatePDUProvider(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbServicePDUProvider *pduProvider = nullptr;
        if (fbAPI->GetServicePDUProvider(path, &pduProvider) != kVIA_OK)
            return CCL_INVALIDPDUPROVIDERPATH;

        return gModule->mPDUProviders.Add(new VServicePDUProvider(pduProvider));
    }

    int32_t VServicePDUProvider::CreatePDUProvider(VIAFbServicePDUProvider *pduProvider) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mPDUProviders.Add(new VServicePDUProvider(pduProvider));
    }

    int32_t VServicePDUProvider::LookupPDUProvider(cclPDUProviderID pduProviderID, VServicePDUProvider *&pduProvider) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        pduProvider = gModule->mPDUProviders.Get(pduProviderID);
        if (pduProvider == nullptr)
            return CCL_INVALIDPDUPROVIDERID;

        return CCL_SUCCESS;
    }

    int32_t VServicePDUProvider::GetNrOfSubscribedConsumers(int32_t *nrOfConsumers) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbServiceConsumerIterator> it;
        if (mPort->GetSubscribedConsumers(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        *nrOfConsumers = 0;
        while (it.mVIAObject->HasMoreConsumers()) {
            ++(*nrOfConsumers);
            it.mVIAObject->SkipConsumer();
        }

        return CCL_SUCCESS;
    }

    int32_t VServicePDUProvider::GetSubscribedConsumers(cclConsumerID *consumerBuffer, int32_t *bufferSize) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbServiceConsumerIterator> it;
        if (mPort->GetSubscribedConsumers(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        cclConsumerID *next = consumerBuffer;
        for (int32_t i = 0; i < *bufferSize; ++i) {
            if (!it.mVIAObject->HasMoreConsumers()) {
                *bufferSize = i;
                break;
            }

            VIAFbServiceConsumer *consumer;
            if (it.mVIAObject->GetNextConsumer(&consumer) != kVIA_OK)
                return CCL_INTERNALERROR;

            *next++ = VConsumer::CreateConsumer(consumer, true);
        }

        return it.mVIAObject->HasMoreConsumers() ? CCL_BUFFERTOSMALL : CCL_SUCCESS;
    }

    // abstract binding specific extensions
    VAbstractServicePDUProvider::VAbstractServicePDUProvider(VIAFbAbstractServicePDUProvider *abstractPDUProvider)
            : VServicePDUProvider(abstractPDUProvider), mAbstractPDUProvider(abstractPDUProvider),
              mPDUSubscribedHandler(nullptr), mPDUUnsubscribedHandler(nullptr), mCallbackHandle(nullptr) {}

    VAbstractServicePDUProvider::~VAbstractServicePDUProvider() {
        if (mCallbackHandle != nullptr) {
            if (mAbstractPDUProvider != nullptr)
                mAbstractPDUProvider->UnregisterObserver(mCallbackHandle);

            mPDUSubscribedHandler = nullptr;
            mPDUUnsubscribedHandler = nullptr;
            mCallbackHandle = nullptr;
        }
    }

    int32_t VAbstractServicePDUProvider::CreatePDUProvider(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbAbstractBinding *abstractBinding;
        if (fbAPI->GetAbstractBinding(&abstractBinding) != kVIA_OK)
            return CCL_INTERNALERROR;

        VIAFbAbstractServicePDUProvider *pduProvider = nullptr;
        if (abstractBinding->GetServicePDUProvider(path, &pduProvider) != kVIA_OK)
            return CCL_INVALIDPDUPROVIDERPATH;

        return gModule->mPDUProviders.Add(new VAbstractServicePDUProvider(pduProvider));
    }

    int32_t VAbstractServicePDUProvider::LookupPDUProvider(cclPDUProviderID pduProviderID,
                                                           VAbstractServicePDUProvider *&pduProvider) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VServicePDUProvider *baseProvider = gModule->mPDUProviders.Get(pduProviderID);
        if (baseProvider == nullptr)
            return CCL_INVALIDPDUPROVIDERID;

        pduProvider = dynamic_cast<VAbstractServicePDUProvider *>(baseProvider);
        if (pduProvider == nullptr)
            return CCL_WRONGBINDING;

        return CCL_SUCCESS;
    }

    int32_t VAbstractServicePDUProvider::UpdateRegistration() {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        VIAResult rv = kVIA_OK;
        if ((mPDUSubscribedHandler != nullptr) || (mPDUUnsubscribedHandler != nullptr)) {
            if (mCallbackHandle == nullptr)
                rv = mAbstractPDUProvider->RegisterObserver(this, &mCallbackHandle);
        } else {
            if (mCallbackHandle != nullptr)
                rv = mAbstractPDUProvider->UnregisterObserver(mCallbackHandle);
            mCallbackHandle = nullptr;
        }
        return (rv == kVIA_OK) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

    int32_t VAbstractServicePDUProvider::SetPDUSubscribedHandler(cclAbstractPDUSubscriptionHandler handler) {
        mPDUSubscribedHandler = handler;
        return UpdateRegistration();
    }

    int32_t VAbstractServicePDUProvider::SetPDUUnsubscribedHandler(cclAbstractPDUSubscriptionHandler handler) {
        mPDUUnsubscribedHandler = handler;
        return UpdateRegistration();
    }

    VIASTDDEF VAbstractServicePDUProvider::OnPDUSubscribed(VIAFbProvidedServicePDU *inPDUPort) {
        if (mPDUSubscribedHandler != nullptr) {
            auto portID = VProvidedServicePDU::CreateProvidedPDU(inPDUPort, false);
            if (portID < 0)
                return kVIA_Failed;

            mPDUSubscribedHandler(portID);
            return VProvidedServicePDU::RemoveProvidedPDU(portID);
        }
        return kVIA_OK;
    }

    VIASTDDEF VAbstractServicePDUProvider::OnPDUUnsubscribed(VIAFbProvidedServicePDU *inPDUPort) {
        if (mPDUUnsubscribedHandler != nullptr) {
            auto portID = VProvidedServicePDU::CreateProvidedPDU(inPDUPort, false);
            if (portID < 0)
                return kVIA_Failed;

            mPDUUnsubscribedHandler(portID);
            return VProvidedServicePDU::RemoveProvidedPDU(portID);
        }
        return kVIA_OK;
    }

#pragma endregion

#pragma region VConsumedEvent Implementation

    // base class for generic event functionality
    VConsumedEvent::VConsumedEvent(VIAFbConsumedEvent *consumedEvent)
            : VPortBase(consumedEvent, true), mConsumedEventID(NDetail::cInvalidCCLObjectID) {}

    VConsumedEvent::~VConsumedEvent() {}

    int32_t VConsumedEvent::CreateConsumedEvent(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbConsumedEvent *consumedEvent = nullptr;
        if (fbAPI->GetConsumedEvent(path, &consumedEvent) != kVIA_OK)
            return CCL_INVALIDCONSUMEDEVENTPATH;

        return gModule->mConsumedEvents.Add(new VConsumedEvent(consumedEvent));
    }

    int32_t VConsumedEvent::CreateConsumedEvent(VIAFbConsumedEvent *consumedEvent) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mConsumedEvents.Add(new VConsumedEvent(consumedEvent));
    }

    int32_t VConsumedEvent::LookupConsumedEvent(cclConsumedEventID consumedEventID, VConsumedEvent *&consumedEvent) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        consumedEvent = gModule->mConsumedEvents.Get(consumedEventID);
        if (consumedEvent == nullptr)
            return CCL_INVALIDCONSUMEDEVENTID;

        return CCL_SUCCESS;
    }

    int32_t VConsumedEvent::SetSubscriptionStateIsolated(bool subscribed) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        auto state = subscribed ? VIAFbSubscriptionState::eSubscribed : VIAFbSubscriptionState::eSubscribable;
        if (mPort->SetSubscriptionStateIsolated(state) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

    // abstract binding specific extensions
    VAbstractConsumedEvent::VAbstractConsumedEvent(VIAFbAbstractConsumedEvent *abstractConsumedEvent)
            : VConsumedEvent(abstractConsumedEvent), mAbstractConsumedEvent(abstractConsumedEvent) {}

    VAbstractConsumedEvent::~VAbstractConsumedEvent() {}

    int32_t VAbstractConsumedEvent::CreateConsumedEvent(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbAbstractBinding *abstractBinding;
        if (fbAPI->GetAbstractBinding(&abstractBinding) != kVIA_OK)
            return CCL_INTERNALERROR;

        VIAFbAbstractConsumedEvent *consumedEvent = nullptr;
        if (abstractBinding->GetConsumedEvent(path, &consumedEvent) != kVIA_OK)
            return CCL_INVALIDCONSUMEDEVENTPATH;

        return gModule->mConsumedEvents.Add(new VAbstractConsumedEvent(consumedEvent));
    }

    int32_t VAbstractConsumedEvent::LookupConsumedEvent(cclConsumedEventID consumedEventID,
                                                        VAbstractConsumedEvent *&consumedEvent) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VConsumedEvent *baseEvent = gModule->mConsumedEvents.Get(consumedEventID);
        if (baseEvent == nullptr)
            return CCL_INVALIDCONSUMEDEVENTID;

        consumedEvent = dynamic_cast<VAbstractConsumedEvent *>(baseEvent);
        if (consumedEvent == nullptr)
            return CCL_WRONGBINDING;

        return CCL_SUCCESS;
    }

#pragma endregion

#pragma region VProvidedEvent Implementation

    VProvidedEvent::VProvidedEvent(VIAFbProvidedEvent *providedEvent, bool release)
            : VPortBase(providedEvent, release), mProvidedEventID(NDetail::cInvalidCCLObjectID) {}

    VProvidedEvent::~VProvidedEvent() {}

    int32_t VProvidedEvent::CreateProvidedEvent(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbProvidedEvent *providedEvent = nullptr;
        if (fbAPI->GetProvidedEvent(path, &providedEvent) != kVIA_OK)
            return CCL_INVALIDPROVIDEDEVENTPATH;

        return gModule->mProvidedEvents.Add(new VProvidedEvent(providedEvent, true));
    }

    int32_t VProvidedEvent::CreateProvidedEvent(VIAFbProvidedEvent *providedEvent, bool release) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mProvidedEvents.Add(new VProvidedEvent(providedEvent, release));
    }

    int32_t VProvidedEvent::LookupProvidedEvent(cclProvidedEventID providedEventID, VProvidedEvent *&providedEvent) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        providedEvent = gModule->mProvidedEvents.Get(providedEventID);
        if (providedEvent == nullptr)
            return CCL_INVALIDPROVIDEDEVENTID;

        return CCL_SUCCESS;
    }

    int32_t VProvidedEvent::RemoveProvidedEvent(cclProvidedEventID providedEventID) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VProvidedEvent *providedEvent = gModule->mProvidedEvents.Get(providedEventID);
        if (providedEvent == nullptr)
            return CCL_INVALIDPROVIDEDEVENTID;

        return gModule->mProvidedEvents.Remove(providedEventID) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

    int32_t VProvidedEvent::SetSubscriptionStateIsolated(bool subscribed) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        auto state = subscribed ? VIAFbSubscriptionState::eSubscribed : VIAFbSubscriptionState::eSubscribable;
        if (mPort->SetSubscriptionStateIsolated(state) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

#pragma endregion

#pragma region VEventProvider Implementation

    // base class for generic event functionality
    VEventProvider::VEventProvider(VIAFbEventProvider *eventProvider)
            : VPortBase(eventProvider, true), mEventProviderID(NDetail::cInvalidCCLObjectID) {}

    VEventProvider::~VEventProvider() {}

    int32_t VEventProvider::CreateEventProvider(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbEventProvider *eventProvider = nullptr;
        if (fbAPI->GetEventProvider(path, &eventProvider) != kVIA_OK)
            return CCL_INVALIDEVENTPROVIDERPATH;

        return gModule->mEventProviders.Add(new VEventProvider(eventProvider));
    }

    int32_t VEventProvider::CreateEventProvider(VIAFbEventProvider *eventProvider) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mEventProviders.Add(new VEventProvider(eventProvider));
    }

    int32_t VEventProvider::LookupEventProvider(cclEventProviderID eventProviderID, VEventProvider *&eventProvider) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        eventProvider = gModule->mEventProviders.Get(eventProviderID);
        if (eventProvider == nullptr)
            return CCL_INVALIDEVENTPROVIDERID;

        return CCL_SUCCESS;
    }

    int32_t VEventProvider::GetNrOfSubscribedConsumers(int32_t *nrOfConsumers) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbServiceConsumerIterator> it;
        if (mPort->GetSubscribedConsumers(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        *nrOfConsumers = 0;
        while (it.mVIAObject->HasMoreConsumers()) {
            ++(*nrOfConsumers);
            it.mVIAObject->SkipConsumer();
        }

        return CCL_SUCCESS;
    }

    int32_t VEventProvider::GetSubscribedConsumers(cclConsumerID *consumerBuffer, int32_t *bufferSize) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbServiceConsumerIterator> it;
        if (mPort->GetSubscribedConsumers(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        cclConsumerID *next = consumerBuffer;
        for (int32_t i = 0; i < *bufferSize; ++i) {
            if (!it.mVIAObject->HasMoreConsumers()) {
                *bufferSize = i;
                break;
            }

            VIAFbServiceConsumer *consumer;
            if (it.mVIAObject->GetNextConsumer(&consumer) != kVIA_OK)
                return CCL_INTERNALERROR;

            *next++ = VConsumer::CreateConsumer(consumer, true);
        }

        return it.mVIAObject->HasMoreConsumers() ? CCL_BUFFERTOSMALL : CCL_SUCCESS;
    }

    // abstract binding specific extensions
    VAbstractEventProvider::VAbstractEventProvider(VIAFbAbstractEventProvider *abstractEventProvider)
            : VEventProvider(abstractEventProvider), mAbstractEventProvider(abstractEventProvider),
              mEventSubscribedHandler(nullptr), mEventUnsubscribedHandler(nullptr), mCallbackHandle(nullptr) {}

    VAbstractEventProvider::~VAbstractEventProvider() {
        if (mCallbackHandle != nullptr) {
            if (mAbstractEventProvider != nullptr)
                mAbstractEventProvider->UnregisterObserver(mCallbackHandle);

            mEventSubscribedHandler = nullptr;
            mEventUnsubscribedHandler = nullptr;
            mCallbackHandle = nullptr;
        }
    }

    int32_t VAbstractEventProvider::CreateEventProvider(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbAbstractBinding *abstractBinding;
        if (fbAPI->GetAbstractBinding(&abstractBinding) != kVIA_OK)
            return CCL_INTERNALERROR;

        VIAFbAbstractEventProvider *eventProvider = nullptr;
        if (abstractBinding->GetEventProvider(path, &eventProvider) != kVIA_OK)
            return CCL_INVALIDEVENTPROVIDERPATH;

        return gModule->mEventProviders.Add(new VAbstractEventProvider(eventProvider));
    }

    int32_t VAbstractEventProvider::LookupEventProvider(cclEventProviderID eventProviderID,
                                                        VAbstractEventProvider *&eventProvider) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VEventProvider *baseProvider = gModule->mEventProviders.Get(eventProviderID);
        if (baseProvider == nullptr)
            return CCL_INVALIDEVENTPROVIDERID;

        eventProvider = dynamic_cast<VAbstractEventProvider *>(baseProvider);
        if (eventProvider == nullptr)
            return CCL_WRONGBINDING;

        return CCL_SUCCESS;
    }

    int32_t VAbstractEventProvider::UpdateRegistration() {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        VIAResult rv = kVIA_OK;
        if ((mEventSubscribedHandler != nullptr) || (mEventUnsubscribedHandler != nullptr)) {
            if (mCallbackHandle == nullptr)
                rv = mAbstractEventProvider->RegisterObserver(this, &mCallbackHandle);
        } else {
            if (mCallbackHandle != nullptr)
                rv = mAbstractEventProvider->UnregisterObserver(mCallbackHandle);
            mCallbackHandle = nullptr;
        }
        return (rv == kVIA_OK) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

    int32_t VAbstractEventProvider::SetEventSubscribedHandler(cclAbstractEventSubscriptionHandler handler) {
        mEventSubscribedHandler = handler;
        return UpdateRegistration();
    }

    int32_t VAbstractEventProvider::SetEventUnsubscribedHandler(cclAbstractEventSubscriptionHandler handler) {
        mEventUnsubscribedHandler = handler;
        return UpdateRegistration();
    }

    VIASTDDEF VAbstractEventProvider::OnEventSubscribed(VIAFbProvidedEvent *inEventPort) {
        if (mEventSubscribedHandler != nullptr) {
            auto portID = VProvidedEvent::CreateProvidedEvent(inEventPort, false);
            if (portID < 0)
                return kVIA_Failed;

            mEventSubscribedHandler(portID);
            return VProvidedEvent::RemoveProvidedEvent(portID);
        }
        return kVIA_OK;
    }

    VIASTDDEF VAbstractEventProvider::OnEventUnsubscribed(VIAFbProvidedEvent *inEventPort) {
        if (mEventUnsubscribedHandler != nullptr) {
            auto portID = VProvidedEvent::CreateProvidedEvent(inEventPort, false);
            if (portID < 0)
                return kVIA_Failed;

            mEventUnsubscribedHandler(portID);
            return VProvidedEvent::RemoveProvidedEvent(portID);
        }
        return kVIA_OK;
    }

#pragma endregion

#pragma region VConsumedField Implementation

    // base class for generic field functionality
    VConsumedField::VConsumedField(VIAFbConsumedField *consumedField)
            : VPortBase(consumedField, true), mConsumedFieldID(NDetail::cInvalidCCLObjectID) {}

    VConsumedField::~VConsumedField() {}

    int32_t VConsumedField::CreateConsumedField(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbConsumedField *consumedField = nullptr;
        auto err = fbAPI->GetConsumedField(path, &consumedField);
        if (err == kVIA_ObjectInvalid)
            return CCL_FIELDNOTSUBSCRIBABLE;
        if (err != kVIA_OK)
            return CCL_INVALIDCONSUMEDFIELDPATH;

        return gModule->mConsumedFields.Add(new VConsumedField(consumedField));
    }

    int32_t VConsumedField::CreateConsumedField(VIAFbConsumedField *consumedField) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mConsumedFields.Add(new VConsumedField(consumedField));
    }

    int32_t VConsumedField::LookupConsumedField(cclConsumedFieldID consumedFieldID, VConsumedField *&consumedField) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        consumedField = gModule->mConsumedFields.Get(consumedFieldID);
        if (consumedField == nullptr)
            return CCL_INVALIDCONSUMEDFIELDID;

        return CCL_SUCCESS;
    }

    int32_t VConsumedField::SetSubscriptionStateIsolated(bool subscribed) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        auto state = subscribed ? VIAFbSubscriptionState::eSubscribed : VIAFbSubscriptionState::eSubscribable;
        if (mPort->SetSubscriptionStateIsolated(state) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

    // abstract binding specific extensions
    VAbstractConsumedField::VAbstractConsumedField(VIAFbAbstractConsumedField *abstractConsumedField)
            : VConsumedField(abstractConsumedField), mAbstractConsumedField(abstractConsumedField) {}

    VAbstractConsumedField::~VAbstractConsumedField() {}

    int32_t VAbstractConsumedField::CreateConsumedField(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbAbstractBinding *abstractBinding;
        if (fbAPI->GetAbstractBinding(&abstractBinding) != kVIA_OK)
            return CCL_INTERNALERROR;

        VIAFbAbstractConsumedField *consumedField = nullptr;
        auto err = abstractBinding->GetConsumedField(path, &consumedField);
        if (err == kVIA_ObjectInvalid)
            return CCL_FIELDNOTSUBSCRIBABLE;
        if (err != kVIA_OK)
            return CCL_INVALIDCONSUMEDFIELDPATH;

        return gModule->mConsumedFields.Add(new VAbstractConsumedField(consumedField));
    }

    int32_t VAbstractConsumedField::LookupConsumedField(cclConsumedFieldID consumedFieldID,
                                                        VAbstractConsumedField *&consumedField) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VConsumedField *baseField = gModule->mConsumedFields.Get(consumedFieldID);
        if (baseField == nullptr)
            return CCL_INVALIDCONSUMEDFIELDID;

        consumedField = dynamic_cast<VAbstractConsumedField *>(baseField);
        if (consumedField == nullptr)
            return CCL_WRONGBINDING;

        return CCL_SUCCESS;
    }

#pragma endregion

#pragma region VProvidedField Implementation

    VProvidedField::VProvidedField(VIAFbProvidedField *providedField, bool release)
            : VPortBase(providedField, release), mProvidedFieldID(NDetail::cInvalidCCLObjectID) {}

    VProvidedField::~VProvidedField() {}

    int32_t VProvidedField::CreateProvidedField(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbProvidedField *providedField = nullptr;
        auto err = fbAPI->GetProvidedField(path, &providedField);
        if (err == kVIA_ObjectInvalid)
            return CCL_FIELDNOTSUBSCRIBABLE;
        if (err != kVIA_OK)
            return CCL_INVALIDPROVIDEDFIELDPATH;

        return gModule->mProvidedFields.Add(new VProvidedField(providedField, true));
    }

    int32_t VProvidedField::CreateProvidedField(VIAFbProvidedField *providedField, bool release) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mProvidedFields.Add(new VProvidedField(providedField, release));
    }

    int32_t VProvidedField::LookupProvidedField(cclProvidedFieldID providedFieldID, VProvidedField *&providedField) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        providedField = gModule->mProvidedFields.Get(providedFieldID);
        if (providedField == nullptr)
            return CCL_INVALIDPROVIDEDFIELDID;

        return CCL_SUCCESS;
    }

    int32_t VProvidedField::RemoveProvidedField(cclProvidedFieldID providedFieldID) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VProvidedField *providedField = gModule->mProvidedFields.Get(providedFieldID);
        if (providedField == nullptr)
            return CCL_INVALIDPROVIDEDFIELDID;

        return gModule->mProvidedFields.Remove(providedFieldID) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

    int32_t VProvidedField::SetSubscriptionStateIsolated(bool subscribed) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        auto state = subscribed ? VIAFbSubscriptionState::eSubscribed : VIAFbSubscriptionState::eSubscribable;
        if (mPort->SetSubscriptionStateIsolated(state) != kVIA_OK)
            return CCL_INTERNALERROR;

        return CCL_SUCCESS;
    }

#pragma endregion

#pragma region VFieldProvider Implementation

    // base class for generic field functionality
    VFieldProvider::VFieldProvider(VIAFbFieldProvider *fieldProvider)
            : VPortBase(fieldProvider, true), mFieldProviderID(NDetail::cInvalidCCLObjectID) {}

    VFieldProvider::~VFieldProvider() {}

    int32_t VFieldProvider::CreateFieldProvider(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbFieldProvider *fieldProvider = nullptr;
        auto err = fbAPI->GetFieldProvider(path, &fieldProvider);
        if (err == kVIA_ObjectInvalid)
            return CCL_FIELDNOTSUBSCRIBABLE;
        if (err != kVIA_OK)
            return CCL_INVALIDFIELDPROVIDERPATH;

        return gModule->mFieldProviders.Add(new VFieldProvider(fieldProvider));
    }

    int32_t VFieldProvider::CreateFieldProvider(VIAFbFieldProvider *fieldProvider) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mFieldProviders.Add(new VFieldProvider(fieldProvider));
    }

    int32_t VFieldProvider::LookupFieldProvider(cclFieldProviderID fieldProviderID, VFieldProvider *&fieldProvider) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        fieldProvider = gModule->mFieldProviders.Get(fieldProviderID);
        if (fieldProvider == nullptr)
            return CCL_INVALIDFIELDPROVIDERID;

        return CCL_SUCCESS;
    }

    int32_t VFieldProvider::GetNrOfSubscribedConsumers(int32_t *nrOfConsumers) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbServiceConsumerIterator> it;
        if (mPort->GetSubscribedConsumers(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        *nrOfConsumers = 0;
        while (it.mVIAObject->HasMoreConsumers()) {
            ++(*nrOfConsumers);
            it.mVIAObject->SkipConsumer();
        }

        return CCL_SUCCESS;
    }

    int32_t VFieldProvider::GetSubscribedConsumers(cclConsumerID *consumerBuffer, int32_t *bufferSize) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbServiceConsumerIterator> it;
        if (mPort->GetSubscribedConsumers(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        cclConsumerID *next = consumerBuffer;
        for (int32_t i = 0; i < *bufferSize; ++i) {
            if (!it.mVIAObject->HasMoreConsumers()) {
                *bufferSize = i;
                break;
            }

            VIAFbServiceConsumer *consumer;
            if (it.mVIAObject->GetNextConsumer(&consumer) != kVIA_OK)
                return CCL_INTERNALERROR;

            *next++ = VConsumer::CreateConsumer(consumer, true);
        }

        return it.mVIAObject->HasMoreConsumers() ? CCL_BUFFERTOSMALL : CCL_SUCCESS;
    }

    // abstract binding specific extensions
    VAbstractFieldProvider::VAbstractFieldProvider(VIAFbAbstractFieldProvider *abstractFieldProvider)
            : VFieldProvider(abstractFieldProvider), mAbstractFieldProvider(abstractFieldProvider),
              mFieldSubscribedHandler(nullptr), mFieldUnsubscribedHandler(nullptr), mCallbackHandle(nullptr) {}

    VAbstractFieldProvider::~VAbstractFieldProvider() {
        if (mCallbackHandle != nullptr) {
            if (mAbstractFieldProvider != nullptr)
                mAbstractFieldProvider->UnregisterObserver(mCallbackHandle);

            mFieldSubscribedHandler = nullptr;
            mFieldUnsubscribedHandler = nullptr;
            mCallbackHandle = nullptr;
        }
    }

    int32_t VAbstractFieldProvider::CreateFieldProvider(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbAbstractBinding *abstractBinding;
        if (fbAPI->GetAbstractBinding(&abstractBinding) != kVIA_OK)
            return CCL_INTERNALERROR;

        VIAFbAbstractFieldProvider *fieldProvider = nullptr;
        auto err = abstractBinding->GetFieldProvider(path, &fieldProvider);
        if (err == kVIA_ObjectInvalid)
            return CCL_FIELDNOTSUBSCRIBABLE;
        if (err != kVIA_OK)
            return CCL_INVALIDFIELDPROVIDERPATH;

        return gModule->mFieldProviders.Add(new VAbstractFieldProvider(fieldProvider));
    }

    int32_t VAbstractFieldProvider::LookupFieldProvider(cclFieldProviderID fieldProviderID,
                                                        VAbstractFieldProvider *&fieldProvider) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VFieldProvider *baseProvider = gModule->mFieldProviders.Get(fieldProviderID);
        if (baseProvider == nullptr)
            return CCL_INVALIDFIELDPROVIDERID;

        fieldProvider = dynamic_cast<VAbstractFieldProvider *>(baseProvider);
        if (fieldProvider == nullptr)
            return CCL_WRONGBINDING;

        return CCL_SUCCESS;
    }

    int32_t VAbstractFieldProvider::UpdateRegistration() {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        VIAResult rv = kVIA_OK;
        if ((mFieldSubscribedHandler != nullptr) || (mFieldUnsubscribedHandler != nullptr)) {
            if (mCallbackHandle == nullptr)
                rv = mAbstractFieldProvider->RegisterObserver(this, &mCallbackHandle);
        } else {
            if (mCallbackHandle != nullptr)
                rv = mAbstractFieldProvider->UnregisterObserver(mCallbackHandle);
            mCallbackHandle = nullptr;
        }
        return (rv == kVIA_OK) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

    int32_t VAbstractFieldProvider::SetFieldSubscribedHandler(cclAbstractFieldSubscriptionHandler handler) {
        mFieldSubscribedHandler = handler;
        return UpdateRegistration();
    }

    int32_t VAbstractFieldProvider::SetFieldUnsubscribedHandler(cclAbstractFieldSubscriptionHandler handler) {
        mFieldUnsubscribedHandler = handler;
        return UpdateRegistration();
    }

    VIASTDDEF VAbstractFieldProvider::OnFieldSubscribed(VIAFbProvidedField *inFieldPort) {
        if (mFieldSubscribedHandler != nullptr) {
            auto portID = VProvidedField::CreateProvidedField(inFieldPort, false);
            if (portID < 0)
                return kVIA_Failed;

            mFieldSubscribedHandler(portID);
            return VProvidedField::RemoveProvidedField(portID);
        }
        return kVIA_OK;
    }

    VIASTDDEF VAbstractFieldProvider::OnFieldUnsubscribed(VIAFbProvidedField *inFieldPort) {
        if (mFieldUnsubscribedHandler != nullptr) {
            auto portID = VProvidedField::CreateProvidedField(inFieldPort, false);
            if (portID < 0)
                return kVIA_Failed;

            mFieldUnsubscribedHandler(portID);
            return VProvidedField::RemoveProvidedField(portID);
        }
        return kVIA_OK;
    }

#pragma endregion

#pragma region VConsumedEventGroup Implementation

    VConsumedEventGroup::VConsumedEventGroup(VIAFbSomeIPConsumedEventGroup *consumedEventGroup)
            : VPortBase(consumedEventGroup, true), mConsumedEventGroupID(NDetail::cInvalidCCLObjectID) {
        consumedEventGroup->GetEventGroupId(&mEventGroupID);
    }

    VConsumedEventGroup::~VConsumedEventGroup() {}

    int32_t VConsumedEventGroup::CreateConsumedEventGroup(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbSomeIPBinding *someIPBinding;
        if (fbAPI->GetSomeIPBinding(&someIPBinding) != kVIA_OK)
            return CCL_INTERNALERROR;

        VIAFbSomeIPConsumedEventGroup *consumedEventGroup = nullptr;
        if (someIPBinding->GetConsumedEventGroup(path, &consumedEventGroup) != kVIA_OK)
            return CCL_INVALIDCONSUMEDEVENTGROUPPATH;

        return gModule->mConsumedEventGroups.Add(new VConsumedEventGroup(consumedEventGroup));
    }

    int32_t VConsumedEventGroup::LookupConsumedEventGroup(cclSomeIPConsumedEventGroupID consumedEventGroupID,
                                                          VConsumedEventGroup *&consumedEventGroup) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        consumedEventGroup = gModule->mConsumedEventGroups.Get(consumedEventGroupID);
        if (consumedEventGroup == nullptr)
            return CCL_INVALIDCONSUMEDEVENTGROUPID;

        return CCL_SUCCESS;
    }

    int32_t VConsumedEventGroup::GetEvents(cclConsumedEventID *eventBuffer, int32_t *bufferSize) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbEventIterator<VIAFbConsumedEvent>> it;
        if (mPort->GetEvents(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        cclConsumedEventID *next = eventBuffer;
        for (int32_t i = 0; i < *bufferSize; ++i) {
            if (!it.mVIAObject->HasMoreEvents()) {
                *bufferSize = i;
                break;
            }

            VIAFbConsumedEvent *consumedEvent;
            if (it.mVIAObject->GetNextEvent(&consumedEvent) != kVIA_OK)
                return CCL_INTERNALERROR;

            *next++ = VConsumedEvent::CreateConsumedEvent(consumedEvent);
        }

        return it.mVIAObject->HasMoreEvents() ? CCL_BUFFERTOSMALL : CCL_SUCCESS;
    }

    int32_t VConsumedEventGroup::GetPDUs(cclPDUReceiverID *pduBuffer, int32_t *bufferSize) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbPDUIterator<VIAFbConsumedServicePDU>> it;
        if (mPort->GetPDUs(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        cclConsumedEventID *next = pduBuffer;
        for (int32_t i = 0; i < *bufferSize; ++i) {
            if (!it.mVIAObject->HasMorePDUs()) {
                *bufferSize = i;
                break;
            }

            VIAFbConsumedServicePDU *consumedPDU;
            if (it.mVIAObject->GetNextPDU(&consumedPDU) != kVIA_OK)
                return CCL_INTERNALERROR;

            *next++ = VConsumedServicePDU::CreateConsumedPDU(consumedPDU);
        }

        return it.mVIAObject->HasMorePDUs() ? CCL_BUFFERTOSMALL : CCL_SUCCESS;
    }

    int32_t VConsumedEventGroup::GetFields(cclConsumedFieldID *fieldBuffer, int32_t *bufferSize) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbFieldIterator<VIAFbConsumedField>> it;
        if (mPort->GetFields(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        cclConsumedFieldID *next = fieldBuffer;
        for (int32_t i = 0; i < *bufferSize; ++i) {
            if (!it.mVIAObject->HasMoreFields()) {
                *bufferSize = i;
                break;
            }

            VIAFbConsumedField *consumedField;
            if (it.mVIAObject->GetNextField(&consumedField) != kVIA_OK)
                return CCL_INTERNALERROR;

            *next++ = VConsumedField::CreateConsumedField(consumedField);
        }

        return it.mVIAObject->HasMoreFields() ? CCL_BUFFERTOSMALL : CCL_SUCCESS;
    }

    bool VConsumedEventGroup::IsEqual(const VPortBase<VIAFbSomeIPConsumedEventGroup> *other) const {
        uint32_t otherID;
        if ((other->mPort == nullptr) || (other->mPort->GetEventGroupId(&otherID) != kVIA_OK))
            return false;

        return VPortBase::IsEqual(other) && (otherID == mEventGroupID);
    }

#pragma endregion

#pragma region VProvidedEventGroup Implementation

    VProvidedEventGroup::VProvidedEventGroup(VIAFbSomeIPProvidedEventGroup *providedEventGroup, bool release)
            : VPortBase(providedEventGroup, release), mProvidedEventGroupID(NDetail::cInvalidCCLObjectID) {
        providedEventGroup->GetEventGroupId(&mEventGroupID);
    }

    VProvidedEventGroup::~VProvidedEventGroup() {}

    int32_t VProvidedEventGroup::CreateProvidedEventGroup(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbSomeIPBinding *someIPBinding;
        if (fbAPI->GetSomeIPBinding(&someIPBinding) != kVIA_OK)
            return CCL_INTERNALERROR;

        VIAFbSomeIPProvidedEventGroup *providedEventGroup = nullptr;
        if (someIPBinding->GetProvidedEventGroup(path, &providedEventGroup) != kVIA_OK)
            return CCL_INVALIDPROVIDEDEVENTGROUPPATH;

        return gModule->mProvidedEventGroups.Add(new VProvidedEventGroup(providedEventGroup, true));
    }

    int32_t VProvidedEventGroup::CreateProvidedEventGroup(VIAFbSomeIPProvidedEventGroup *providedEventGroup) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        return gModule->mProvidedEventGroups.Add(new VProvidedEventGroup(providedEventGroup, false));
    }

    int32_t VProvidedEventGroup::LookupProvidedEventGroup(cclSomeIPProvidedEventGroupID providedEventGroupID,
                                                          VProvidedEventGroup *&providedEventGroup) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        providedEventGroup = gModule->mProvidedEventGroups.Get(providedEventGroupID);
        if (providedEventGroup == nullptr)
            return CCL_INVALIDPROVIDEDEVENTGROUPID;

        return CCL_SUCCESS;
    }

    int32_t VProvidedEventGroup::RemoveProvidedEventGroup(cclSomeIPProvidedEventGroupID providedEventGroupID) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VProvidedEventGroup *providedEventGroup = gModule->mProvidedEventGroups.Get(providedEventGroupID);
        if (providedEventGroup == nullptr)
            return CCL_INVALIDPROVIDEDEVENTGROUPID;

        return gModule->mProvidedEventGroups.Remove(providedEventGroupID) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

    int32_t VProvidedEventGroup::GetEvents(cclProvidedEventID *eventBuffer, int32_t *bufferSize) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbEventIterator<VIAFbProvidedEvent>> it;
        if (mPort->GetEvents(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        cclProvidedEventID *next = eventBuffer;
        for (int32_t i = 0; i < *bufferSize; ++i) {
            if (!it.mVIAObject->HasMoreEvents()) {
                *bufferSize = i;
                break;
            }

            VIAFbProvidedEvent *providedEvent;
            if (it.mVIAObject->GetNextEvent(&providedEvent) != kVIA_OK)
                return CCL_INTERNALERROR;

            *next++ = VProvidedEvent::CreateProvidedEvent(providedEvent, true);
        }

        return it.mVIAObject->HasMoreEvents() ? CCL_BUFFERTOSMALL : CCL_SUCCESS;
    }

    int32_t VProvidedEventGroup::GetPDUs(cclPDUSenderID *pduBuffer, int32_t *bufferSize) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbPDUIterator<VIAFbProvidedServicePDU>> it;
        if (mPort->GetPDUs(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        cclPDUSenderID *next = pduBuffer;
        for (int32_t i = 0; i < *bufferSize; ++i) {
            if (!it.mVIAObject->HasMorePDUs()) {
                *bufferSize = i;
                break;
            }

            VIAFbProvidedServicePDU *providedPDU;
            if (it.mVIAObject->GetNextPDU(&providedPDU) != kVIA_OK)
                return CCL_INTERNALERROR;

            *next++ = VProvidedServicePDU::CreateProvidedPDU(providedPDU, true);
        }

        return it.mVIAObject->HasMorePDUs() ? CCL_BUFFERTOSMALL : CCL_SUCCESS;
    }

    int32_t VProvidedEventGroup::GetFields(cclProvidedFieldID *fieldBuffer, int32_t *bufferSize) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbFieldIterator<VIAFbProvidedField>> it;
        if (mPort->GetFields(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        cclProvidedFieldID *next = fieldBuffer;
        for (int32_t i = 0; i < *bufferSize; ++i) {
            if (!it.mVIAObject->HasMoreFields()) {
                *bufferSize = i;
                break;
            }

            VIAFbProvidedField *providedField;
            if (it.mVIAObject->GetNextField(&providedField) != kVIA_OK)
                return CCL_INTERNALERROR;

            *next++ = VProvidedField::CreateProvidedField(providedField, true);
        }

        return it.mVIAObject->HasMoreFields() ? CCL_BUFFERTOSMALL : CCL_SUCCESS;
    }

    bool VProvidedEventGroup::IsEqual(const VPortBase<VIAFbSomeIPProvidedEventGroup> *other) const {
        uint32_t otherID;
        if ((other->mPort == nullptr) || (other->mPort->GetEventGroupId(&otherID) != kVIA_OK))
            return false;

        return VPortBase::IsEqual(other) && (otherID == mEventGroupID);
    }

#pragma endregion

#pragma region VEventGroupProvider Implementation

    VEventGroupProvider::VEventGroupProvider(VIAFbSomeIPEventGroupProvider *eventGroupProvider)
            : VPortBase(eventGroupProvider, true), mEventGroupProviderID(NDetail::cInvalidCCLObjectID),
              mEventGroupSubscribedHandler(nullptr), mEventGroupUnsubscribedHandler(nullptr),
              mCallbackHandle(nullptr) {}

    VEventGroupProvider::~VEventGroupProvider() {
        if (mCallbackHandle != nullptr) {
            if (mPort != nullptr)
                mPort->UnregisterObserver(mCallbackHandle);

            mEventGroupSubscribedHandler = nullptr;
            mEventGroupUnsubscribedHandler = nullptr;
            mCallbackHandle = nullptr;
        }
    }

    int32_t VEventGroupProvider::CreateEventGroupProvider(const char *path) {
        if (path == nullptr)
            return CCL_PARAMETERINVALID;

        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        VIAFbViaService *fbAPI = GetFunctionBusAPI();
        if (fbAPI == nullptr)
            return CCL_NOTIMPLEMENTED;

        VIAFbSomeIPBinding *someIPBinding;
        if (fbAPI->GetSomeIPBinding(&someIPBinding) != kVIA_OK)
            return CCL_INTERNALERROR;

        VIAFbSomeIPEventGroupProvider *eventGroupProvider = nullptr;
        if (someIPBinding->GetEventGroupProvider(path, &eventGroupProvider) != kVIA_OK)
            return CCL_INVALIDEVENTGROUPPROVIDERPATH;

        return gModule->mEventGroupProviders.Add(new VEventGroupProvider(eventGroupProvider));
    }

    int32_t VEventGroupProvider::LookupEventGroupProvider(cclSomeIPEventGroupProviderID eventGroupProviderID,
                                                          VEventGroupProvider *&eventGroupProvider) {
        if (gModule == nullptr)
            return CCL_INTERNALERROR;

        eventGroupProvider = gModule->mEventGroupProviders.Get(eventGroupProviderID);
        if (eventGroupProvider == nullptr)
            return CCL_INVALIDEVENTGROUPPROVIDERID;

        return CCL_SUCCESS;
    }

    int32_t VEventGroupProvider::GetNrOfSubscribedConsumers(int32_t *nrOfConsumers) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbServiceConsumerIterator> it;
        if (mPort->GetSubscribedConsumers(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        *nrOfConsumers = 0;
        while (it.mVIAObject->HasMoreConsumers()) {
            ++(*nrOfConsumers);
            it.mVIAObject->SkipConsumer();
        }

        return CCL_SUCCESS;
    }

    int32_t VEventGroupProvider::GetSubscribedConsumers(cclConsumerID *consumerBuffer, int32_t *bufferSize) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbServiceConsumerIterator> it;
        if (mPort->GetSubscribedConsumers(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        cclConsumerID *next = consumerBuffer;
        for (int32_t i = 0; i < *bufferSize; ++i) {
            if (!it.mVIAObject->HasMoreConsumers()) {
                *bufferSize = i;
                break;
            }

            VIAFbServiceConsumer *consumer;
            if (it.mVIAObject->GetNextConsumer(&consumer) != kVIA_OK)
                return CCL_INTERNALERROR;

            *next++ = VConsumer::CreateConsumer(consumer, true);
        }

        return it.mVIAObject->HasMoreConsumers() ? CCL_BUFFERTOSMALL : CCL_SUCCESS;
    }

    int32_t VEventGroupProvider::GetEvents(cclEventProviderID *eventBuffer, int32_t *bufferSize) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbEventIterator<VIAFbEventProvider>> it;
        if (mPort->GetEvents(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        cclEventProviderID *next = eventBuffer;
        for (int32_t i = 0; i < *bufferSize; ++i) {
            if (!it.mVIAObject->HasMoreEvents()) {
                *bufferSize = i;
                break;
            }

            VIAFbEventProvider *eventProvider;
            if (it.mVIAObject->GetNextEvent(&eventProvider) != kVIA_OK)
                return CCL_INTERNALERROR;

            *next++ = VEventProvider::CreateEventProvider(eventProvider);
        }

        return it.mVIAObject->HasMoreEvents() ? CCL_BUFFERTOSMALL : CCL_SUCCESS;
    }

    int32_t VEventGroupProvider::GetPDUs(cclPDUProviderID *pduBuffer, int32_t *bufferSize) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbPDUIterator<VIAFbServicePDUProvider>> it;
        if (mPort->GetPDUs(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        cclPDUProviderID *next = pduBuffer;
        for (int32_t i = 0; i < *bufferSize; ++i) {
            if (!it.mVIAObject->HasMorePDUs()) {
                *bufferSize = i;
                break;
            }

            VIAFbServicePDUProvider *pduProvider;
            if (it.mVIAObject->GetNextPDU(&pduProvider) != kVIA_OK)
                return CCL_INTERNALERROR;

            *next++ = VServicePDUProvider::CreatePDUProvider(pduProvider);
        }

        return it.mVIAObject->HasMorePDUs() ? CCL_BUFFERTOSMALL : CCL_SUCCESS;
    }

    int32_t VEventGroupProvider::GetFields(cclFieldProviderID *fieldBuffer, int32_t *bufferSize) {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        NDetail::VIAObjectGuard <VIAFbFieldIterator<VIAFbFieldProvider>> it;
        if (mPort->GetFields(&(it.mVIAObject)) != kVIA_OK)
            return CCL_INTERNALERROR;

        cclFieldProviderID *next = fieldBuffer;
        for (int32_t i = 0; i < *bufferSize; ++i) {
            if (!it.mVIAObject->HasMoreFields()) {
                *bufferSize = i;
                break;
            }

            VIAFbFieldProvider *fieldProvider;
            if (it.mVIAObject->GetNextField(&fieldProvider) != kVIA_OK)
                return CCL_INTERNALERROR;

            *next++ = VFieldProvider::CreateFieldProvider(fieldProvider);
        }

        return it.mVIAObject->HasMoreFields() ? CCL_BUFFERTOSMALL : CCL_SUCCESS;
    }

    int32_t VEventGroupProvider::UpdateRegistration() {
        if (mPort == nullptr)
            return CCL_INTERNALERROR;

        VIAResult rv = kVIA_OK;
        if ((mEventGroupSubscribedHandler != nullptr) || (mEventGroupUnsubscribedHandler != nullptr)) {
            if (mCallbackHandle == nullptr)
                rv = mPort->RegisterObserver(this, &mCallbackHandle);
        } else {
            if (mCallbackHandle != nullptr)
                rv = mPort->UnregisterObserver(mCallbackHandle);
            mCallbackHandle = nullptr;
        }
        return (rv == kVIA_OK) ? CCL_SUCCESS : CCL_INTERNALERROR;
    }

    int32_t VEventGroupProvider::SetEventGroupSubscribedHandler(cclSomeIPEventGroupSubscriptionHandler handler) {
        mEventGroupSubscribedHandler = handler;
        return UpdateRegistration();
    }

    int32_t VEventGroupProvider::SetEventGroupUnsubscribedHandler(cclSomeIPEventGroupSubscriptionHandler handler) {
        mEventGroupUnsubscribedHandler = handler;
        return UpdateRegistration();
    }

    VIASTDDEF VEventGroupProvider::OnEventGroupSubscribed(VIAFbSomeIPProvidedEventGroup *inEventGroupPort) {
        if (mEventGroupSubscribedHandler != nullptr) {
            auto portID = VProvidedEventGroup::CreateProvidedEventGroup(inEventGroupPort);
            if (portID < 0)
                return kVIA_Failed;

            mEventGroupSubscribedHandler(portID);
            return VProvidedEventGroup::RemoveProvidedEventGroup(portID);
        }
        return kVIA_OK;
    }

    VIASTDDEF VEventGroupProvider::OnEventGroupUnsubscribed(VIAFbSomeIPProvidedEventGroup *inEventGroupPort) {
        if (mEventGroupUnsubscribedHandler != nullptr) {
            auto portID = VProvidedEventGroup::CreateProvidedEventGroup(inEventGroupPort);
            if (portID < 0)
                return kVIA_Failed;

            mEventGroupUnsubscribedHandler(portID);
            return VProvidedEventGroup::RemoveProvidedEventGroup(portID);
        }
        return kVIA_OK;
    }

    bool VEventGroupProvider::IsEqual(const VPortBase<VIAFbSomeIPEventGroupProvider> *other) const {
        uint32_t otherID;
        if ((other->mPort == nullptr) || (other->mPort->GetEventGroupId(&otherID) != kVIA_OK))
            return false;

        return VPortBase::IsEqual(other) && (otherID == mEventGroupID);
    }

#pragma endregion

#pragma region VNodeLayerModule Implementation

    // ============================================================================
    // Implementation of class VNodeLayerModule
    // ============================================================================

    VNodeLayerModule::VNodeLayerModule()
            : mMeasurementPreStartHandler(nullptr), mMeasurementStartHandler(nullptr), mMeasurementStopHandler(nullptr),
              mDllUnloadHandler(nullptr) {}


    VIASTDDEF VNodeLayerModule::Init() {
        return kVIA_OK;
    }


    VIASTDDEF VNodeLayerModule::GetVersion(char *buffer, int32 bufferLength) {
        if (buffer != nullptr && bufferLength > 0) {
            UtilStringCopy(buffer, bufferLength, "CANaylzer/CANoe C Library Version 2.0");
        }
        return kVIA_OK;
    }


    VIASTDDEF VNodeLayerModule::GetModuleParameters(char *pathBuff, int32 pathBuffLength,
                                                    char *versionBuff, int32 versionBuffLength) {
#if defined(_WIN32)
        {
            ::GetModuleFileNameA(gDLLInstanceHandle, pathBuff, pathBuffLength);
        }
#elif defined(__linux__)
                                                                                                                                {
      Dl_info info;
      int rc = dladdr(reinterpret_cast<void*>(&VIASetService), &info);  // address of any symbol inside this library
      if ((rc != 0) && (info.dli_fname != nullptr) && (pathBuff != 0) && (pathBuffLength > 0))
      {
        // modulePath = info.dli_fname;
        strncpy(pathBuff, info.dli_fname, pathBuffLength-1);
        pathBuff[pathBuffLength-1] = char(0);
      }
    }
#endif
        GetVersion(versionBuff, versionBuffLength);
        return kVIA_OK;
    }


    VIASTDDEF VNodeLayerModule::CreateObject(VIANodeLayerApi **object, VIANode *node,
                                             int32 argc, char *argv[]) {
        uint32 busType;
        node->GetBusType(&busType);

        VIAChannel channel;
        node->GetChannel(&channel);

        std::string busName;
        if (gHasBusNames) {
            char text[256];
            VIAResult result = node->GetBusName(text, 256);
            if (result == kVIA_OK) {
                busName = text;
            }
        }

        VIABus *bus = nullptr;
        if (busType == kVIA_CAN) {
            gVIAService->GetBusInterface(&bus, node, kVIA_CAN, VIACANMajorVersion, VIACANMinorVersion);
        } else if (busType == kVIA_LIN) {
            gVIAService->GetBusInterface(&bus, node, kVIA_LIN, VIALINMajorVersion, VIALINMinorVersion);
        } else if (busType == kVIA_Ethernet) {
            gVIAService->GetBusInterface(&bus, node, kVIA_Ethernet, VIAEthernetMajorVersion, VIAEthernetMinorVersion);
        }

        VNodeLayer *layer = new VNodeLayer(node, busType, channel, bus);
        *object = layer;

        if (busType == kVIA_CAN && channel >= 1 && channel <= cMaxChannel) {
            gCanBusContext[channel].mBus = static_cast<VIACan *>(bus);
            gCanBusContext[channel].mNode = node;
            gCanBusContext[channel].mLayer = layer;
            gCanBusContext[channel].mBusName = busName;
        } else if (busType == kVIA_LIN && channel >= 1 && channel <= cMaxChannel) {
            gLinBusContext[channel].mBus = static_cast<VIALin *>(bus);;
            gLinBusContext[channel].mNode = node;
            gLinBusContext[channel].mLayer = layer;
            gLinBusContext[channel].mBusName = busName;
        } else if (busType == kVIA_Ethernet && channel >= 1 && channel <= cMaxChannel) {
            gEthernetBusContext[channel].mBus = static_cast<VIAEthernetBus *>(bus);;
            gEthernetBusContext[channel].mNode = node;
            gEthernetBusContext[channel].mLayer = layer;
            gEthernetBusContext[channel].mBusName = busName;
        }

        if (gMasterLayer == nullptr) {
            gMasterLayer = layer;
        }

        return kVIA_OK;
    }


    VIASTDDEF VNodeLayerModule::ReleaseObject(VIANodeLayerApi *object) {
        VNodeLayer *layer = static_cast<VNodeLayer *>(object);
        uint32 busType = layer->mBusType;
        VIAChannel channel = layer->mChannel;

        if (gMasterLayer == layer) {
            gMasterLayer = nullptr;
            for (size_t i = 0; i < mSignal.size(); i++) {
                if (mSignal[i] != nullptr) {
                    gSignalAccessAPI->ReleaseSignal(mSignal[i]->mViaSignal);
                    delete mSignal[i];
                    mSignal[i] = nullptr;
                }
            }
        }

        if (busType == kVIA_CAN && channel >= 1 && channel <= cMaxChannel) {
            gCanBusContext[channel].mBus = nullptr;
            gCanBusContext[channel].mNode = nullptr;
            gCanBusContext[channel].mLayer = nullptr;
            gCanBusContext[channel].mBusName.clear();
        } else if (busType == kVIA_LIN && channel >= 1 && channel <= cMaxChannel) {
            gLinBusContext[channel].mBus = nullptr;
            gLinBusContext[channel].mNode = nullptr;
            gLinBusContext[channel].mLayer = nullptr;
            gLinBusContext[channel].mBusName.clear();
        } else if (busType == kVIA_Ethernet && channel >= 1 && channel <= cMaxChannel) {
            gEthernetBusContext[channel].mBus = nullptr;
            gLinBusContext[channel].mNode = nullptr;
            gLinBusContext[channel].mLayer = nullptr;
            gLinBusContext[channel].mBusName.clear();
        }

        if (layer->mBus != nullptr) {
            gVIAService->ReleaseBusInterface(layer->mBus);
        }

        delete layer;

        return kVIA_OK;
    }


    VIASTDDEF VNodeLayerModule::GetNodeInfo(const char *nodename,
                                            char *shortNameBuf, int32 shortBufLength,
                                            char *longNameBuf, int32 longBufLength) {
        if (shortNameBuf != nullptr && shortBufLength > 0) {
            shortNameBuf[0] = char(0);
        }

        if (longNameBuf != nullptr && longBufLength > 0) {
            longNameBuf[0] = char(0);
        }

        return kVIA_OK;
    }


    VIASTDDEF VNodeLayerModule::InitMeasurement() {
        return kVIA_OK;
    }


    VIASTDDEF VNodeLayerModule::EndMeasurement() {
        gState = eLoaded;

        // delete Timers
        for (size_t i = 0; i < mTimer.size(); i++) {
            VTimer *timer = mTimer[i];
            if (timer != nullptr) {
                if (timer->mViaTimer != nullptr) {
                    VIAResult rc = gVIAService->ReleaseTimer(timer->mViaTimer);
                    timer->mViaTimer = nullptr;
                }
                delete timer;
                mTimer[i] = nullptr;
            }
        }
        mTimer.clear();

        // delete system variables
        for (size_t i = 0; i < mSysVar.size(); i++) {
            VSysVar *sysVar = mSysVar[i];
            if (sysVar != nullptr) {
                if (sysVar->mViaSysVarMember != nullptr) {
                    VIAResult rc = sysVar->mViaSysVarMember->Release();
                    sysVar->mViaSysVarMember = nullptr;
                }
                if (sysVar->mViaSysVar != nullptr) {
                    VIAResult rc = sysVar->mViaSysVar->Release();
                    sysVar->mViaSysVar = nullptr;
                }
                delete sysVar;
                mSysVar[i] = nullptr;
            }
        }
        mSysVar.clear();

        // delete message requests for CAN
        for (size_t i = 0; i < mCanMessageRequests.size(); i++) {
            VCanMessageRequest *request = mCanMessageRequests[i];
            if (request != nullptr) {
                if (request->mHandle != nullptr && request->mContext != nullptr) {
                    request->mContext->mBus->ReleaseRequest(request->mHandle);
                    request->mHandle = nullptr;
                }
                delete request;
                mCanMessageRequests[i] = nullptr;
            }
        }
        mCanMessageRequests.clear();

        // delete message requests for LIN
        for (size_t i = 0; i < mLinMessageRequests.size(); i++) {
            VLinMessageRequest *request = mLinMessageRequests[i];
            if (request != nullptr) {
                if (request->mHandle != nullptr && request->mContext != nullptr) {
                    request->mContext->mBus->ReleaseRequest(request->mHandle);
                    request->mHandle = nullptr;
                }
                delete request;
                mLinMessageRequests[i] = nullptr;
            }
        }
        mLinMessageRequests.clear();

        // delete error requests for LIN
        for (size_t i = 0; i < mLinErrorRequests.size(); i++) {
            VLinErrorRequest *request = mLinErrorRequests[i];
            if (request != nullptr) {
                if (request->mContext != nullptr) {
                    for (size_t j = 0; j < VLinErrorRequest::kMaxHandles; ++j) {
                        if (request->mHandle[j] != nullptr) {
                            request->mContext->mBus->ReleaseRequest(request->mHandle[j]);
                            request->mHandle[j] = nullptr;
                        }
                    }
                }
                delete request;
                mLinErrorRequests[i] = nullptr;
            }
        }
        mLinErrorRequests.clear();

        // delete slave response changed requests for LIN
        for (size_t i = 0; i < mLinSlaveResponseChange.size(); i++) {
            VLinSlaveResponseChange *request = mLinSlaveResponseChange[i];
            if (request != nullptr) {
                if (request->mHandle != nullptr && request->mContext != nullptr) {
                    request->mContext->mBus->ReleaseRequest(request->mHandle);
                    request->mHandle = nullptr;
                }
                delete request;
                mLinSlaveResponseChange[i] = nullptr;
            }
        }
        mLinSlaveResponseChange.clear();

        // delete pre-dispatch message requests for LIN
        for (size_t i = 0; i < mLinPreDispatchMessage.size(); i++) {
            VLinPreDispatchMessage *request = mLinPreDispatchMessage[i];
            if (request != nullptr) {
                if (request->mHandle != nullptr && request->mContext != nullptr) {
                    request->mContext->mBus->ReleaseRequest(request->mHandle);
                    request->mHandle = nullptr;
                }
                delete request;
                mLinPreDispatchMessage[i] = nullptr;
            }
        }
        mLinPreDispatchMessage.clear();

        // delete sleep mode event requests for LIN
        for (size_t i = 0; i < mLinSleepModeEvent.size(); i++) {
            VLinSleepModeEvent *request = mLinSleepModeEvent[i];
            if (request != nullptr) {
                if (request->mHandle != nullptr && request->mContext != nullptr) {
                    request->mContext->mBus->ReleaseRequest(request->mHandle);
                    request->mHandle = nullptr;
                }
                delete request;
                mLinSleepModeEvent[i] = nullptr;
            }
        }
        mLinSleepModeEvent.clear();

        // delete wakeup frame requests for LIN
        for (size_t i = 0; i < mLinWakeupFrame.size(); i++) {
            VLinWakeupFrame *request = mLinWakeupFrame[i];
            if (request != nullptr) {
                if (request->mHandle != nullptr && request->mContext != nullptr) {
                    request->mContext->mBus->ReleaseRequest(request->mHandle);
                    request->mHandle = nullptr;
                }
                delete request;
                mLinWakeupFrame[i] = nullptr;
            }
        }
        mLinWakeupFrame.clear();

        // delete packet requests for Ethernet
        for (size_t i = 0; i < mEthernetPacketRequests.size(); i++) {
            VEthernetPacketRequest *request = mEthernetPacketRequests[i];
            if (request != nullptr) {
                if (request->mHandle != nullptr && request->mContext != nullptr) {
                    request->mContext->mBus->ReleaseRequest(request->mHandle);
                    request->mHandle = nullptr;
                }
                delete request;
                mEthernetPacketRequests[i] = nullptr;
            }
        }
        mEthernetPacketRequests.clear();

        // delete function bus entities
        VService::Cleanup();
        mValues.Clear();
        mFunctions.Clear();
        mCallContexts.Clear();
        mConsumers.Clear();
        mProviders.Clear();
        mConsumedServices.Clear();
        mProvidedServices.Clear();
        mAddresses.Clear();
        mPDUSender.Clear();
        mPDUReceiver.Clear();
        mPDUProviders.Clear();
        mConsumedEvents.Clear();
        mProvidedEvents.Clear();
        mEventProviders.Clear();
        mConsumedFields.Clear();
        mProvidedFields.Clear();
        mFieldProviders.Clear();
        mConsumedEventGroups.Clear();
        mProvidedEventGroups.Clear();
        mEventGroupProviders.Clear();

        return kVIA_OK;
    }


    VIASTDDEF VNodeLayerModule::GetNodeInfoEx(VIDBNodeDefinition *nodeDefinition,
                                              char *shortNameBuf, int32 shortBufLength,
                                              char *longNameBuf, int32 longBufLength) {
        if (shortNameBuf != nullptr && shortBufLength > 0) {
            shortNameBuf[0] = char(0);
        }

        if (longNameBuf != nullptr && longBufLength > 0) {
            longNameBuf[0] = char(0);
        }

        return kVIA_OK;
    }

    VIASTDDEF VNodeLayerModule::DoInformOfChange(VIDBNodeDefinition *nodeDefinition,
                                                 const uint32 changeFlags,
                                                 const char *simBusName,
                                                 const VIABusInterfaceType busType,
                                                 const VIAChannel channel,
                                                 const char *oldName,
                                                 const char *newName,
                                                 const int32 bValue) {
        return kVIA_OK;
    }

#pragma endregion

#pragma region VNodeLayer Implementation

    // ============================================================================
    // Implementation of class VNodeLayer
    // ============================================================================

    VNodeLayer::VNodeLayer(VIANode *node, uint32 busType, VIAChannel channel, VIABus *bus)
            : mNode(node), mBusType(busType), mChannel(channel), mBus(bus) {}


    VNodeLayer::~VNodeLayer() {}


    VIASTDDEF  VNodeLayer::GetNode(VIANode **node) {
        *node = mNode;
        return kVIA_OK;
    }


    VIASTDDEF  VNodeLayer::InitMeasurement() {
        if (gState == eLoaded) {
            VIAResult rc = gVIAService->GetSignalAccessApi(&gSignalAccessAPI, mNode, 2, 10);

            gState = eInitMeasurement;
            if (gModule->mMeasurementPreStartHandler != nullptr) {
                gModule->mMeasurementPreStartHandler();
            }
        }
        return kVIA_OK;
    }


    VIASTDDEF  VNodeLayer::StartMeasurement() {
        if (gState == eInitMeasurement) {
            gState = eRunMeasurement;
            if (gModule->mMeasurementStartHandler != nullptr) {
                gModule->mMeasurementStartHandler();
            }
        }
        return kVIA_OK;
    }


    VIASTDDEF  VNodeLayer::StopMeasurement() {
        if (gState == eRunMeasurement) {
            gState = eStopMeasurement;
            if (gModule->mMeasurementStopHandler != nullptr) {
                gModule->mMeasurementStopHandler();
            }
        }
        return kVIA_OK;
    }


    VIASTDDEF VNodeLayer::EndMeasurement() {
        return kVIA_OK;
    }


    VIASTDDEF VNodeLayer::InitMeasurementComplete() {
        return kVIA_OK;
    }


    VIASTDDEF VNodeLayer::PreInitMeasurement() {
        return kVIA_OK;
    }

    VIASTDDEF VNodeLayer::PreStopMeasurement() {
        return kVIA_OK;
    }

#pragma endregion
}


using namespace CANalyzerCANoeCLibrary;

#pragma region Node Layer DLL

// ============================================================================
// Define the entry point for the DLL application (Windows Platform only).
// ============================================================================

#if defined(_WIN32)

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
    if (DLL_PROCESS_ATTACH == ul_reason_for_call) {
        gDLLInstanceHandle = static_cast<HINSTANCE>(hModule);
    }

    return TRUE;
}

#endif

// ============================================================================
// NodeLayer DLL Interface Section
// ============================================================================



VIACLIENT(void) VIARequiredVersion(int32 *majorversion, int32 *minorversion) {
    *majorversion = VIAMajorVersion;
    *minorversion = 40;  // VIAMinorVersion;
}



VIACLIENT(void) VIADesiredVersion(int32 *majorversion, int32 *minorversion) {
    *majorversion = VIAMajorVersion;
    *minorversion = VIAMinorVersion;
}



VIACLIENT(void) VIASetService(VIAService *service) {
    VIAResult rc;
    gVIAService = service;
    rc = gVIAService->GetSystemVariablesRootNamespace(gSysVarRoot);

    {
        int32 major, minor, patchlevel;
        rc = gVIAService->GetVersion(&major, &minor, &patchlevel);
        if (rc == kVIA_OK) {
            gVIAMinorVersion = minor;
        }

        // C-Libraries are executed in a context of a dummy node. Because the C-Library was feature was created primary for
        // CANalyzer, this special context has no bus names in older versions of CANoe.
        gHasBusNames = minor >= 105;
    }
}



VIACLIENT(VIAModuleApi*) VIAGetModuleApi(int32 argc, char *argv[]) {
    gState = eLoaded;

    if (gModule == nullptr) {
        gModule = new VNodeLayerModule();
    }

    cclOnDllLoad();

    return gModule;
} // VIAGetModuleApi



VIACLIENT(void) VIAReleaseModuleApi(VIAModuleApi *api) {
    gState = eUnloaded;

    if (gModule != nullptr && gModule->mDllUnloadHandler != nullptr) {
        gModule->mDllUnloadHandler();
    }

    if (gVIAService != nullptr && gSysVarRoot != nullptr) {
        gSysVarRoot->Release();
    }

    if (gVIAService != nullptr && gADASService != nullptr) {
        gVIAService->ReleaseADASService(gADASService);
    }
    gSysVarRoot = nullptr;

    gFunctionBusAPI = nullptr;

    gADASService = nullptr;

    gVIAService = nullptr;


    delete gModule;
    gModule = nullptr;
} // VIAReleaseModuleApi


#if defined(_WIN32)
extern "C" __declspec(dllexport) int32 __stdcall NLGetModuleOptions(int32 option)
#elif defined(__linux__)
                                                                                                                        extern "C" __attribute__((visibility("default"))) int32  NLGetModuleOptions(int32 option)
#else
extern "C" int32 NLGetModuleOptions(int32 option)
#endif
{
    if (option == kVIA_ModuleOption_LoadOption)  // Load Option (0 default, 1 permanent, 2 defer)
    {
        return kVIA_ModuleOption_LoadOption_Defer;
    } else if (option ==
               kVIA_GetModuleOption_DLLType) // DLL Type Option (0 default, 1 standard node layer, 2 C library)
    {
        return kVIA_ModuleOption_DLLType_CLibrary; // DLL Type C Library
    } else {
        return 0;
    }
}

#pragma endregion


#pragma region Global API Functions

// ============================================================================
// CCL API functions
// ============================================================================



void cclSetMeasurementPreStartHandler(void (*function)()) {
    if (gModule != nullptr) {
        gModule->mMeasurementPreStartHandler = function;
    }
}


void cclSetMeasurementStartHandler(void (*function)()) {
    if (gModule != nullptr) {
        gModule->mMeasurementStartHandler = function;
    }
}


void cclSetMeasurementStopHandler(void (*function)()) {
    if (gModule != nullptr) {
        gModule->mMeasurementStopHandler = function;
    }
}


void cclSetDllUnloadHandler(void (*function)()) {
    if (gModule != nullptr) {
        gModule->mDllUnloadHandler = function;
    }
}


void cclWrite(const char *text) {
    if (gVIAService != nullptr && text != nullptr) {
        VIAResult rc = gVIAService->WriteString(text);
    }
}


void cclPrintf(const char *format, ...) {
    char buffer[4096];
    va_list arg;
    int result;

            va_start(arg, format);
#if defined(_MSC_VER)
    result = _vsnprintf_s(buffer, 4096, _TRUNCATE, format, arg);
#else
    result = vsnprintf(buffer, 4096, format, arg);
#endif
            va_end(arg);


    if (result < 0) {
        return;
    }

    if (gVIAService != nullptr) {
        VIAResult rc = gVIAService->WriteString(buffer);
    }
};


int32_t cclGetUserFilePath(const char *filename,
                           char *pathBuffer,
                           int32_t pathBufferLength) {
    if (gVIAService == nullptr) {
        return CCL_INTERNALERROR;
    }
    if (gVIAMinorVersion < 41) {
        return CCL_NOTIMPLEMENTED;
    }
    if (filename == nullptr && strlen(filename) == 0) {
        return CCL_INVALIDNAME;
    }

    VIAResult rc = gVIAService->GetUserFilePath(filename, pathBuffer, pathBufferLength);
    switch (rc) {
        case kVIA_OK:
            return CCL_SUCCESS;
        case kVIA_ObjectNotFound:
            return CCL_USERFILENOTDEFINED;
        case kVIA_ParameterInvalid:
            return CCL_PARAMETERINVALID;
        case kVIA_BufferToSmall:
            return CCL_BUFFERTOSMALL;
        case kVIA_ServiceNotRunning:
            return CCL_WRONGSTATE;
        case kVIA_Failed:
            return CCL_INTERNALERROR;
        default:
            return CCL_INTERNALERROR;
    }
}

#pragma endregion

#pragma region Time & Timer

// ============================================================================
// Time & Timer
// ============================================================================


int64_t cclTimeSeconds(int64_t seconds) {
    return seconds * 1000000000LL;
}


int64_t cclTimeMilliseconds(int64_t milliseconds) {
    return milliseconds * 1000000LL;
}


int64_t cclTimeMicroseconds(int64_t microseconds) {
    return microseconds * 1000LL;
}


int32_t cclTimerCreate(void (*function)(int64_t time, int32_t timerID)) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement)
    if (gModule == nullptr || gVIAService == nullptr) {
        return CCL_INTERNALERROR;
    }
    if (function == nullptr) {
        return CCL_INVALIDFUNCTIONPOINTER;
    }

    VTimer *cclTimer = new VTimer;
    VIATimer *viaTimer = nullptr;

    VIAResult rc = gVIAService->CreateTimer(&viaTimer, gMasterLayer->mNode, cclTimer, "C-Library Timer");
    if (rc != kVIA_OK) {
        delete cclTimer;
        return CCL_INTERNALERROR;
    }

    cclTimer->mCallbackFunction = function;
    cclTimer->mViaTimer = viaTimer;
    cclTimer->mTimerID = static_cast<int32_t>(gModule->mTimer.size());
    gModule->mTimer.push_back(cclTimer);

    return cclTimer->mTimerID;
}


int32_t cclTimerSet(int32_t timerID, int64_t nanoseconds) {
    CCL_STATECHECK(eRunMeasurement)
    if (gModule == nullptr) {
        return CCL_INTERNALERROR;
    }
    if (nanoseconds <= 0) {
        return CCL_INVALIDTIME;
    }

    if (timerID < 0 || timerID >= static_cast<int32_t>(gModule->mTimer.size())) {
        return CCL_INVALIDTIMERID;
    }

    VTimer *cclTimer = gModule->mTimer[timerID];
    if (cclTimer == nullptr || cclTimer->mTimerID != timerID) {
        return CCL_INVALIDTIMERID;
    }

    VIAResult rc = cclTimer->mViaTimer->SetTimer(nanoseconds);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclTimerCancel(int32_t timerID) {
    CCL_STATECHECK(eRunMeasurement)
    if (gModule == nullptr) {
        return CCL_INTERNALERROR;
    }

    if (timerID < 0 || timerID >= static_cast<int32_t>(gModule->mTimer.size())) {
        return CCL_INVALIDTIMERID;
    }

    VTimer *cclTimer = gModule->mTimer[timerID];
    if (cclTimer == nullptr || cclTimer->mTimerID != timerID) {
        return CCL_INVALIDTIMERID;
    }

    VIAResult rc = cclTimer->mViaTimer->CancelTimer();
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}

int32_t cclIncrementTimerBase(int64_t newTimeBaseTicks, int32_t numberOfSteps) {
    if (gVIAMinorVersion < 54) {
        return CCL_NOTIMPLEMENTED;
    }

    CCL_STATECHECK(eRunMeasurement)
    if (gVIAService) {
        VIAResult rc = gVIAService->IncrementTimerBase(newTimeBaseTicks, numberOfSteps);
        switch (rc) {
            case kVIA_OK:
                return CCL_SUCCESS;
            case kVIA_ParameterInvalid:
                return CCL_PARAMETERINVALID;
            case kVIA_Failed:
                return CCL_WRONGSTATE;
            default:
                return CCL_INTERNALERROR;
        }
    }
    return CCL_INTERNALERROR;
}

int32_t cclIsRunningInSlaveMode(bool *isSlaveMode) {
    VIAResult rc = gVIAService->IsSlaveMode(isSlaveMode);
    // function always succeeds
    return CCL_SUCCESS;
}


#pragma endregion

#pragma region System Variables

// ============================================================================
// System Variables
// ============================================================================


int32_t cclSysVarGetID(const char *systemVariableName) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)
    if (gSysVarRoot == nullptr || gModule == nullptr) {
        return CCL_INTERNALERROR;
    }
    if (systemVariableName == nullptr && strlen(systemVariableName) == 0) {
        return CCL_INVALIDNAME;
    }

    bool isStructMember = false;
    const char *varName = nullptr;
    const char *memberName = nullptr;
    char *nonMemberPart = nullptr;
    {
        size_t pos = strcspn(systemVariableName, ".[");
        size_t len = strlen(systemVariableName);
        if (pos < len) {
            isStructMember = true;
            if (systemVariableName[pos] == '.') {
                memberName = systemVariableName + pos + 1; // exclude the '.'
            } else {
                memberName = systemVariableName + pos; // include the '['
            }

            size_t varNameLength = pos;
            if (varNameLength == 0) {
                return CCL_INVALIDNAME;
            }
            if (strlen(memberName) == 0) {
                return CCL_INVALIDNAME;
            }
            if (gVIAMinorVersion < 50) {
                return CCL_NOTIMPLEMENTED;
            }
            nonMemberPart = (char *) malloc(varNameLength + 1);
            if (nonMemberPart == nullptr) {
                return CCL_INTERNALERROR;
            }
            UtilStringCopyN(nonMemberPart, varNameLength + 1, systemVariableName, varNameLength);
            varName = nonMemberPart;
        } else {
            isStructMember = false;
            varName = systemVariableName;
            memberName = nullptr;
        }
    }

    VIASystemVariable *viaSysVar = nullptr;
    VIAResult rc = gSysVarRoot->GetVariable(varName, viaSysVar);
    if (isStructMember) {
        free(nonMemberPart);
        nonMemberPart = nullptr;
        varName = nullptr;;
    }
    if (rc != kVIA_OK || viaSysVar == nullptr) {
        if (rc == kVIA_ObjectNotFound) {
            return CCL_SYSVARNOTDEFINED;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    VIASystemVariableMember *viaSysVarMember = nullptr;
    if (isStructMember) {
        rc = viaSysVar->GetMember(memberName, &viaSysVarMember);
        if (rc != kVIA_OK || viaSysVar == nullptr) {
            viaSysVar->Release();
            viaSysVar = nullptr;
            if (rc == kVIA_ObjectNotFound) {
                return CCL_SYSVARNOTDEFINED;
            } else {
                return CCL_INTERNALERROR;
            }
        }
    }

    VSysVar *cclSysVar = new VSysVar;
    cclSysVar->mViaSysVar = viaSysVar;
    cclSysVar->mViaSysVarMember = viaSysVarMember;
    cclSysVar->mVariableName = (varName != nullptr) ? varName : "";
    cclSysVar->mMemberName = (memberName != nullptr) ? memberName : "";
    cclSysVar->mCallbackFunction = nullptr;
    cclSysVar->mSysVarID = static_cast<int32_t>(gModule->mSysVar.size());
    gModule->mSysVar.push_back(cclSysVar);

    return cclSysVar->mSysVarID;
}


int32_t cclSysVarGetType(int32_t sysVarID) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIASysVarType type;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        VIAResult rc;
        rc = cclSysVar->mViaSysVarMember->GetType(&type);
        if (rc != kVIA_OK) {
            if (rc == kVIA_ObjectNotFound) {
                return CCL_SYSVARNOTDEFINED;
            } else {
                return CCL_INTERNALERROR;
            }
        }
    } else {
        VIAResult rc;
        rc = cclSysVar->mViaSysVar->GetType(&type);
        if (rc != kVIA_OK) {
            if (rc == kVIA_ObjectInvalid) {
                return CCL_SYSVARNOTDEFINED;
            } else {
                return CCL_INTERNALERROR;
            }
        }
    }

    switch (type) {
        case kVIA_SVInteger:
            return CCL_SYSVAR_INTEGER;
        case kVIA_SVFloat:
            return CCL_SYSVAR_FLOAT;
        case kVIA_SVString:
            return CCL_SYSVAR_STRING;
        case kVIA_SVIntegerArray:
            return CCL_SYSVAR_INTEGERARRAY;
        case kVIA_SVFloatArray:
            return CCL_SYSVAR_FLOATARRAY;
        case kVIA_SVData:
            return CCL_SYSVAR_DATA;
        case kVIA_SVStruct:
            return CCL_SYSVAR_STRUCT;
        case kVIA_SVGenericArray:
            return CCL_SYSVAR_GENERICARRAY;
        default:
            return CCL_INTERNALERROR;
    }
}


int32_t cclSysVarSetHandler(int32_t sysVarID, void (*function)(int64_t time, int32_t sysVarID)) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    cclSysVar->mCallbackFunction = function;
    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        VIAOnSysVarMember *sink = (cclSysVar->mCallbackFunction == nullptr) ? nullptr : cclSysVar;
        rc = cclSysVar->mViaSysVarMember->SetSink(sink);
    } else {
        VIAOnSysVar *sink = (cclSysVar->mCallbackFunction == nullptr) ? nullptr : cclSysVar;
        rc = cclSysVar->mViaSysVar->SetSink(sink);
    }

    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarGetArraySize(int32_t sysVarID) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    // check that system variable has an array type
    VIASysVarType varType;
    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        rc = cclSysVar->mViaSysVarMember->GetType(&varType);
    } else {
        rc = cclSysVar->mViaSysVar->GetType(&varType);
    }
    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else {
            return CCL_INTERNALERROR;
        }
    }
    if (varType != kVIA_SVIntegerArray && varType != kVIA_SVFloatArray && varType != kVIA_SVData &&
        varType != kVIA_SVGenericArray) {
        return CCL_WRONGTYPE;
    }

    int32 size;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        rc = cclSysVar->mViaSysVarMember->GetArraySize(&size);
        if (rc != kVIA_OK) {
            if (rc == kVIA_ObjectInvalid) {
                return CCL_SYSVARNOTDEFINED;
            } else {
                return CCL_INTERNALERROR;
            }
        }
        if (varType == kVIA_SVGenericArray && size > 0) {
            std::string elementName = cclSysVar->mMemberName;
            elementName += "[0]";

            VIASystemVariableMember *arrayElement;
            rc = cclSysVar->mViaSysVar->GetMember(elementName.c_str(), &arrayElement);
            if (rc != kVIA_OK) {
                if (rc == kVIA_ObjectInvalid) {
                    return CCL_SYSVARNOTDEFINED;
                } else {
                    return CCL_INTERNALERROR;
                }
            }

            int32 bitLength = 0;
            rc = arrayElement->GetBitLength(&bitLength);
            arrayElement->Release();
            if (rc != kVIA_OK) {
                if (rc == kVIA_ObjectInvalid) {
                    return CCL_SYSVARNOTDEFINED;
                } else {
                    return CCL_INTERNALERROR;
                }
            }
            size = (8 * size) / bitLength;
        }
    } else {
        rc = cclSysVar->mViaSysVar->GetArraySize(&size);
        if (rc != kVIA_OK) {
            if (rc == kVIA_ObjectInvalid) {
                return CCL_SYSVARNOTDEFINED;
            } else {
                return CCL_INTERNALERROR;
            }
        }

        if (varType == kVIA_SVGenericArray && size > 0) {
            std::string elementName = cclSysVar->mMemberName;
            elementName += "[0]";

            VIASystemVariableMember *arrayElement;
            rc = cclSysVar->mViaSysVar->GetMember(elementName.c_str(), &arrayElement);
            if (rc != kVIA_OK) {
                if (rc == kVIA_ObjectInvalid) {
                    return CCL_SYSVARNOTDEFINED;
                } else {
                    return CCL_INTERNALERROR;
                }
            }

            int32 bitLength = 0;
            rc = arrayElement->GetBitLength(&bitLength);
            arrayElement->Release();
            if (rc != kVIA_OK) {
                if (rc == kVIA_ObjectInvalid) {
                    return CCL_SYSVARNOTDEFINED;
                } else {
                    return CCL_INTERNALERROR;
                }
            }
            size = (8 * size) / bitLength;
        }
    }

    return size;
}


int32_t cclSysVarSetInteger(int32_t sysVarID, int32_t x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVInteger, true);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        rc = cclSysVar->mViaSysVarMember->SetInteger(static_cast<::int64>(x), nullptr);
    } else {
        rc = cclSysVar->mViaSysVar->SetInteger(x, nullptr);
    }
    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarGetInteger(int32_t sysVarID, int32_t *x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVInteger, false);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        ::int64 y;
        rc = cclSysVar->mViaSysVarMember->GetInteger(&y);
        *x = static_cast<int32_t>(y);
    } else {
        rc = cclSysVar->mViaSysVar->GetInteger(reinterpret_cast<int32 *>(x));
    }

    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarSetInteger64(int32_t sysVarID, int64_t x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVInteger, true);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        rc = cclSysVar->mViaSysVarMember->SetInteger(x, nullptr);
    } else {
        rc = cclSysVar->mViaSysVar->SetIntegerEx(x, nullptr);
    }
    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarGetInteger64(int32_t sysVarID, int64_t *x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVInteger, false);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        rc = cclSysVar->mViaSysVarMember->GetInteger(x);
    } else {
        rc = cclSysVar->mViaSysVar->GetIntegerEx(x);
    }

    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarSetFloat(int32_t sysVarID, double x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVFloat, true);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        rc = cclSysVar->mViaSysVarMember->SetFloat(x, nullptr);
    } else {
        rc = cclSysVar->mViaSysVar->SetFloat(x, nullptr);
    }
    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarGetFloat(int32_t sysVarID, double *x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVFloat, false);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        rc = cclSysVar->mViaSysVarMember->GetFloat(x);
    } else {
        rc = rc = cclSysVar->mViaSysVar->GetFloat(x);
    }
    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarSetString(int32_t sysVarID, const char *text) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVString, true);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        rc = cclSysVar->mViaSysVarMember->SetString(text, nullptr);
    } else {
        rc = cclSysVar->mViaSysVar->SetString(text, nullptr);
    }
    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarGetString(int32_t sysVarID, char *buffer, int32_t bufferLenght) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVString, false);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }
    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        rc = cclSysVar->mViaSysVarMember->GetString(buffer, bufferLenght);
    } else {
        rc = cclSysVar->mViaSysVar->GetString(buffer, bufferLenght);
    }
    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else if (rc == kVIA_BufferToSmall) {
            return CCL_BUFFERTOSMALL;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarSetIntegerArray(int32_t sysVarID, const int32_t *x, int32_t length) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVIntegerArray, true);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc;
    int32 size;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        VIASysVarType varType;
        {
            VIAResult rc = cclSysVar->mViaSysVarMember->GetType(&varType);
            if (rc != kVIA_OK) {
                return CCL_INTERNALERROR;
            }
        }
        if (varType == kVIA_SVGenericArray) {
            size = cclSysVarGetArraySize(sysVarID);
            if (size < 0) {
                return size;
            }
            rc = kVIA_OK;
        } else {
            rc = cclSysVar->mViaSysVarMember->GetArraySize(&size);
        }
    } else {
        rc = cclSysVar->mViaSysVar->GetArraySize(&size);
    }
    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else {
            return CCL_INTERNALERROR;
        }
    }
    if (length != size) {
        return CCL_WRONGARRAYSIZE;
    }

    if (cclSysVar->mViaSysVarMember != nullptr) {
        VIASysVarType varType;
        rc = cclSysVar->mViaSysVarMember->GetType(&varType);
        if (rc != kVIA_OK) {
            return CCL_INTERNALERROR;
        }

        if (varType == kVIA_SVGenericArray) {
            char memberName[1024];
            char arryIndex[24];
            rc = cclSysVar->mViaSysVarMember->GetName(memberName, 1000);
            if (rc != kVIA_OK) {
                return CCL_INTERNALERROR;
            }
            size_t memberNameLength = strlen(memberName);

            for (int32_t i = 0; i < length; i++) {
#if defined(_MSC_VER)
                sprintf_s(arryIndex, 24, "[%d]", i);
                strcpy_s(memberName + memberNameLength, 24, arryIndex);
#else
                                                                                                                                        snprintf(arryIndex, 24, "[%d]", i);
        strcpy(memberName + memberNameLength,arryIndex);
#endif

                VIASystemVariableMember *element = nullptr;
                cclSysVar->mViaSysVar->GetMember(memberName, &element);
                if (rc != kVIA_OK) {
                    return CCL_INTERNALERROR;
                }

                VIASysVarType elementType;
                rc = element->GetType(&elementType);
                if (rc != kVIA_OK) {
                    element->Release();
                    return CCL_INTERNALERROR;
                }
                if (elementType != kVIA_SVInteger) {
                    element->Release();
                    return CCL_WRONGTYPE;
                }

                ::int64 elementValue = static_cast<int64>(x[i]);;
                rc = element->SetInteger(elementValue, nullptr);
                if (rc != kVIA_OK) {
                    element->Release();
                    return CCL_INTERNALERROR;
                }

                element->Release();
            }
        } else if (varType == kVIA_SVIntegerArray) {
            ::int64 *buffer = static_cast<::int64 *>( alloca(length * sizeof(::int64)));
            for (int32_t i = 0; i < length; i++) {
                buffer[i] = static_cast<::int64>(x[i]);
            }
            rc = cclSysVar->mViaSysVarMember->SetIntegerArray(buffer, length, nullptr);
            if (rc != kVIA_OK) {
                if (rc == kVIA_ObjectInvalid) {
                    return CCL_SYSVARNOTDEFINED;
                } else {
                    return CCL_INTERNALERROR;
                }
            }
        } else {
            return CCL_WRONGTYPE;
        }
    } else {
        rc = cclSysVar->mViaSysVar->SetIntegerArray(reinterpret_cast<const int32 *>(x), length, nullptr);
        if (rc != kVIA_OK) {
            if (rc == kVIA_ObjectInvalid) {
                return CCL_SYSVARNOTDEFINED;
            } else {
                return CCL_INTERNALERROR;
            }
        }
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarGetIntegerArray(int32_t sysVarID, int32_t *x, int32_t length) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVIntegerArray, false);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    if (length < 0) {
        return CCL_BUFFERTOSMALL;
    }

    if (cclSysVar->mViaSysVarMember != nullptr) {
        VIASysVarType varType;
        {
            VIAResult rc = cclSysVar->mViaSysVarMember->GetType(&varType);
            if (rc != kVIA_OK) {
                return CCL_INTERNALERROR;
            }
        }
        if (varType == kVIA_SVGenericArray) {
            int32 arraySize = cclSysVarGetArraySize(sysVarID);
            if (arraySize < 0) {
                return arraySize;
            }
            if (arraySize > length) {
                return CCL_BUFFERTOSMALL;
            }

            char memberName[1024];
            char arryIndex[24];
            VIAResult rc = cclSysVar->mViaSysVarMember->GetName(memberName, 1000);
            if (rc != kVIA_OK) {
                return CCL_INTERNALERROR;
            }
            size_t memberNameLength = strlen(memberName);

            for (int32_t i = 0; i < arraySize; i++) {
#if defined(_MSC_VER)
                sprintf_s(arryIndex, 24, "[%d]", i);
                strcpy_s(memberName + memberNameLength, 24, arryIndex);
#else
                                                                                                                                        snprintf(arryIndex, 24, "[%d]", i);
        strcpy(memberName + memberNameLength, arryIndex);
#endif

                VIASystemVariableMember *element = nullptr;
                cclSysVar->mViaSysVar->GetMember(memberName, &element);
                if (rc != kVIA_OK) {
                    return CCL_INTERNALERROR;
                }

                VIASysVarType elementType;
                rc = element->GetType(&elementType);
                if (rc != kVIA_OK) {
                    element->Release();
                    return CCL_INTERNALERROR;
                }
                if (elementType != kVIA_SVInteger) {
                    element->Release();
                    return CCL_WRONGTYPE;
                }

                ::int64 elementValue;
                rc = element->GetInteger(&elementValue);
                if (rc != kVIA_OK) {
                    element->Release();
                    return CCL_INTERNALERROR;
                }
                x[i] = static_cast<int32_t>(elementValue);

                element->Release();
            }
        } else if (varType == kVIA_SVIntegerArray) {
            ::int64 *buffer = static_cast<::int64 *>( alloca(length * sizeof(::int64)));
            VIAResult rc = cclSysVar->mViaSysVarMember->GetIntegerArray(buffer, length);
            if (rc != kVIA_OK) {
                if (rc == kVIA_ObjectInvalid) {
                    return CCL_SYSVARNOTDEFINED;
                } else if (rc == kVIA_BufferToSmall) {
                    return CCL_BUFFERTOSMALL;
                } else {
                    return CCL_INTERNALERROR;
                }
            }
            for (int32_t i = 0; i < length; i++) {
                x[i] = static_cast<int32_t>(buffer[i]);
            }
        } else {
            return CCL_WRONGTYPE;
        }
    } else {
        VIAResult rc = cclSysVar->mViaSysVar->GetIntegerArray(reinterpret_cast<int32 *>(x), length);
        if (rc != kVIA_OK) {
            if (rc == kVIA_ObjectInvalid) {
                return CCL_SYSVARNOTDEFINED;
            } else if (rc == kVIA_BufferToSmall) {
                return CCL_BUFFERTOSMALL;
            } else {
                return CCL_INTERNALERROR;
            }
        }
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarSetFloatArray(int32_t sysVarID, const double *x, int32_t length) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVFloatArray, true);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    int32 size;
    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        rc = cclSysVar->mViaSysVarMember->GetArraySize(&size);
    } else {
        rc = cclSysVar->mViaSysVar->GetArraySize(&size);
    }
    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else {
            return CCL_INTERNALERROR;
        }
    }
    if (length != size) {
        return CCL_WRONGARRAYSIZE;
    }

    if (cclSysVar->mViaSysVarMember != nullptr) {
        rc = cclSysVar->mViaSysVarMember->SetFloatArray(x, length, nullptr);
    } else {
        rc = cclSysVar->mViaSysVar->SetFloatArray(x, length, nullptr);
    }

    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarGetFloatArray(int32_t sysVarID, double *x, int32_t length) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVFloatArray, false);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        rc = cclSysVar->mViaSysVarMember->GetFloatArray(x, length);
    } else {
        rc = cclSysVar->mViaSysVar->GetFloatArray(x, length);
    }

    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else if (rc == kVIA_BufferToSmall) {
            return CCL_BUFFERTOSMALL;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarSetData(int32_t sysVarID, const uint8_t *x, int32_t length) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    if (gVIAMinorVersion < 42) {
        return CCL_NOTIMPLEMENTED;
    }

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVData, true);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        rc = cclSysVar->mViaSysVarMember->SetData(x, length, nullptr);
    } else {
        rc = cclSysVar->mViaSysVar->SetData(x, length, nullptr);
    }
    if (rc != kVIA_OK) {
        if (rc == kVIA_ObjectInvalid) {
            return CCL_SYSVARNOTDEFINED;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarGetData(int32_t sysVarID, uint8_t *x, int32_t length) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    if (gVIAMinorVersion < 42) {
        return CCL_NOTIMPLEMENTED;
    }

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar, kVIA_SVData, false);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    int32 bytesCopied;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        int32 arraySize = 0;
        VIAResult rc = cclSysVar->mViaSysVarMember->GetArraySize(&arraySize);
        if (rc != kVIA_OK) {
            if (rc == kVIA_ObjectInvalid) {
                return CCL_SYSVARNOTDEFINED;
            } else {
                return CCL_INTERNALERROR;
            }
        }
        if (arraySize > length) {
            return CCL_BUFFERTOSMALL;
        }
        rc = cclSysVar->mViaSysVarMember->GetData(x, arraySize);
        if (rc != kVIA_OK) {
            if (rc == kVIA_ObjectInvalid) {
                return CCL_SYSVARNOTDEFINED;
            } else {
                return CCL_INTERNALERROR;
            }
        }
        bytesCopied = arraySize;
    } else {
        VIAResult rc = cclSysVar->mViaSysVar->GetData(x, length, &bytesCopied);
        if (rc != kVIA_OK) {
            if (rc == kVIA_ObjectInvalid) {
                return CCL_SYSVARNOTDEFINED;
            } else if (rc == kVIA_BufferToSmall) {
                return CCL_BUFFERTOSMALL;
            } else {
                return CCL_INTERNALERROR;
            }
        }
    }

    return bytesCopied;
}


int32_t cclSysVarSetPhysical(int32_t sysVarID, double x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar);
        if (result != CCL_SUCCESS) {
            return result;
        }
        result = cclSysVar->CheckWriteable();
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        VIASysVarType varType;
        rc = cclSysVar->mViaSysVarMember->GetType(&varType);
        if (rc != kVIA_OK) {
            return CCL_INTERNALERROR;
        }
        if (varType != kVIA_SVInteger && varType != kVIA_SVFloat) {
            return CCL_WRONGTYPE;
        }

        rc = cclSysVar->mViaSysVarMember->SetPhysicalValue(x, nullptr);
        if (rc != kVIA_OK) {
            if (rc == kVIA_ObjectInvalid) {
                return CCL_SYSVARNOTDEFINED;
            } else {
                return CCL_INTERNALERROR;
            }
        }
    } else {
        return CCL_NOTIMPLEMENTED;  // physical value is only implemented for member of struct
    }

    return CCL_SUCCESS;
}


int32_t cclSysVarGetPhysical(int32_t sysVarID, double *x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSysVar *cclSysVar = nullptr;
    {
        int32_t result = VSysVar::LookupVariable(sysVarID, cclSysVar);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc;
    if (cclSysVar->mViaSysVarMember != nullptr) {
        VIASysVarType varType;
        rc = cclSysVar->mViaSysVarMember->GetType(&varType);
        if (rc != kVIA_OK) {
            return CCL_INTERNALERROR;
        }
        if (varType != kVIA_SVInteger && varType != kVIA_SVFloat) {
            return CCL_WRONGTYPE;
        }

        rc = cclSysVar->mViaSysVarMember->GetPhysicalValue(x);
        if (rc != kVIA_OK) {
            if (rc == kVIA_ObjectInvalid) {
                return CCL_SYSVARNOTDEFINED;
            } else {
                return CCL_INTERNALERROR;
            }
        }
    } else {
        return CCL_NOTIMPLEMENTED;  // physical value is only implemented for member of struct
    }

    return CCL_SUCCESS;
}

#pragma endregion

#pragma region Signals

// ============================================================================
// Signal
// ============================================================================

int32_t cclSignalGetID(const char *signalname) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)
    if (gModule == nullptr) {
        return CCL_INTERNALERROR;
    }
    if (gSignalAccessAPI == nullptr) {
        return CCL_NOTIMPLEMENTED;
    }
    if (signalname == nullptr && strlen(signalname) == 0) {
        return CCL_INVALIDNAME;
    }

    VIASignal *viaSignal = nullptr;
    VIAResult rc = gSignalAccessAPI->GetSignalByName(&viaSignal, signalname);
    if (rc != kVIA_OK || viaSignal == nullptr) {
        if (rc == kVIA_ObjectNotFound) {
            return CCL_SIGNALNOTDEFINED;
        } else if (rc == kVIA_SignalAmbiguous) {
            return CCL_SIGNALAMBIGUOUS;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    VSignal *cclSignal = new VSignal;
    cclSignal->mViaSignal = viaSignal;
    cclSignal->mSignalID = static_cast<int32_t>(gModule->mSignal.size());
    gModule->mSignal.push_back(cclSignal);

    return cclSignal->mSignalID;
}


extern int32_t cclSignalSetHandler(int32_t signalID, void (*function)(int32_t signalID)) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSignal *cclSignal = nullptr;
    {
        int32_t result = VSignal::LookupSignal(signalID, cclSignal);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    cclSignal->mCallbackFunction = function;
    VIAOnSignal *sink = (cclSignal->mCallbackFunction == nullptr) ? nullptr : cclSignal;
    VIAResult rc = cclSignal->mViaSignal->SetSinkOnChangeOnly(sink, nullptr);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclSignalGetRxPhysDouble(int32_t signalID, double *value) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSignal *cclSignal = nullptr;
    {
        int32_t result = VSignal::LookupSignal(signalID, cclSignal);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc = cclSignal->mViaSignal->GetPhysicalValue(value);
    switch (rc) {
        case kVIA_OK:
            return CCL_SUCCESS;
        case kVIA_ObjectInvalid:
            return CCL_NOTIMPLEMENTED; // signal has no physical value
        default:
            return CCL_INTERNALERROR;
    }
}


int32_t cclSignalGetRxRawInteger(int32_t signalID, int64_t *value) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VSignal *cclSignal = nullptr;
    {
        int32_t result = VSignal::LookupSignal(signalID, cclSignal);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc = cclSignal->mViaSignal->GetRawValueInt64(value);
    switch (rc) {
        case kVIA_OK:
            return CCL_SUCCESS;
        case kVIA_WrongSignalType:
            return CCL_WRONGTYPE;
        default:
            return CCL_INTERNALERROR;
    }
}


int32_t cclSignalGetRxRawDouble(int32_t signalID, double *value) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VSignal *cclSignal = nullptr;
    {
        int32_t result = VSignal::LookupSignal(signalID, cclSignal);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc = cclSignal->mViaSignal->GetRawValueDouble(value);
    switch (rc) {
        case kVIA_OK:
            return CCL_SUCCESS;
        case kVIA_WrongSignalType:
            return CCL_WRONGTYPE;
        default:
            return CCL_INTERNALERROR;
    }
}


int32_t cclSignalSetTxPhysDouble(int32_t signalID, double value) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement)

    VSignal *cclSignal = nullptr;
    {
        int32_t result = VSignal::LookupSignal(signalID, cclSignal);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc = cclSignal->mViaSignal->SetPhysicalValue(value);
    switch (rc) {
        case kVIA_OK:
            return CCL_SUCCESS;
        case kVIA_ObjectInvalid:
            return CCL_NOTIMPLEMENTED; // signal is read only or has no physical value
        default:
            return CCL_INTERNALERROR;
    }
}


int32_t cclSignalSetTxRawInteger(int32_t signalID, int64_t value) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VSignal *cclSignal = nullptr;
    {
        int32_t result = VSignal::LookupSignal(signalID, cclSignal);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc = cclSignal->mViaSignal->SetRawValueInt64(value);
    switch (rc) {
        case kVIA_OK:
            return CCL_SUCCESS;
        case kVIA_WrongSignalType:
            return CCL_WRONGTYPE;
        default:
            return CCL_INTERNALERROR;
    }
}


int32_t cclSignalSetTxRawDouble(int32_t signalID, double value) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VSignal *cclSignal = nullptr;
    {
        int32_t result = VSignal::LookupSignal(signalID, cclSignal);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc = cclSignal->mViaSignal->SetRawValueDouble(value);
    switch (rc) {
        case kVIA_OK:
            return CCL_SUCCESS;
        case kVIA_WrongSignalType:
            return CCL_WRONGTYPE;
        default:
            return CCL_INTERNALERROR;
    }
}


int32_t cclSignalGetTxPhysDouble(int32_t signalID, double *value) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VSignal *cclSignal = nullptr;
    {
        int32_t result = VSignal::LookupSignal(signalID, cclSignal);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc = cclSignal->mViaSignal->GetTxPhysicalValue(value);
    switch (rc) {
        case kVIA_OK:
            return CCL_SUCCESS;
        case kVIA_WrongSignalType:
            return CCL_WRONGTYPE;
        case kVIA_ObjectInvalid:
            return CCL_NOTIMPLEMENTED; // signal is read only or has no physical value
        default:
            return CCL_INTERNALERROR;
    }
}


int32_t cclSignalGetTxRawInteger(int32_t signalID, int64_t *value) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VSignal *cclSignal = nullptr;
    {
        int32_t result = VSignal::LookupSignal(signalID, cclSignal);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc = cclSignal->mViaSignal->GetTxRawValueInt64(value);
    switch (rc) {
        case kVIA_OK:
            return CCL_SUCCESS;
        case kVIA_WrongSignalType:
            return CCL_WRONGTYPE;
        default:
            return CCL_INTERNALERROR;
    }
}


int32_t cclSignalGetTxRawDouble(int32_t signalID, double *value) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VSignal *cclSignal = nullptr;
    {
        int32_t result = VSignal::LookupSignal(signalID, cclSignal);
        if (result != CCL_SUCCESS) {
            return result;
        }
    }

    VIAResult rc = cclSignal->mViaSignal->GetTxRawValueDouble(value);
    switch (rc) {
        case kVIA_OK:
            return CCL_SUCCESS;
        case kVIA_WrongSignalType:
            return CCL_WRONGTYPE;
        default:
            return CCL_INTERNALERROR;
    }
}

#pragma endregion

#pragma region CAN Messages

// ============================================================================
// CAN
// ============================================================================


int32_t
cclCanOutputMessage(int32_t channel, uint32_t identifier, uint32_t flags, uint8_t dataLength, const uint8_t data[]) {
    CCL_STATECHECK(eRunMeasurement)

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    VIACan *canBus = gCanBusContext[channel].mBus;
    if (canBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    uint32 viaFlags =
            ((flags & CCL_CANFLAGS_RTR) ? kVIA_CAN_RemoteFrame : 0) |
            ((flags & CCL_CANFLAGS_WAKEUP) ? kVIA_CAN_Wakeup : 0) |
            ((flags & CCL_CANFLAGS_FDF) ? kVIA_CAN_EDL : 0) |
            ((flags & CCL_CANFLAGS_BRS) ? kVIA_CAN_BRS : 0);
    uint8 txReqCount = 0;  // value zero means no limitation of transmit attempts
    VIAResult rc = canBus->OutputMessage3(channel, identifier, viaFlags, txReqCount, dataLength, data);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclCanSetMessageHandler(int32_t channel, uint32_t identifier, void (*function)(cclCanMessage *message)) {
    CCL_STATECHECK(eInitMeasurement)

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gCanBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    if (function == nullptr) {
        return CCL_INVALIDFUNCTIONPOINTER;
    }

    uint8 requestType = (identifier == CCL_CAN_ALLMESSAGES) ? kVIA_AllId : kVIA_OneId;

    VCanMessageRequest *request = new VCanMessageRequest;
    request->mCallbackFunction = function;
    request->mContext = &gCanBusContext[channel];
    request->mHandle = nullptr;


    // 假如将这里的channel 改为 VIAChannel kVIA_WildcardChannel = 0xFFFFFF，是否就可以接收所有通道的消息？
    VIAResult rc = gCanBusContext[channel].mBus->CreateMessageRequest3(&(request->mHandle), request, requestType,
                                                                       identifier, channel, 0);
    if (rc != kVIA_OK) {
        delete request;
        if (rc == kVIA_ServiceNotRunning) {
            return CCL_WRONGSTATE;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    gModule->mCanMessageRequests.push_back(request);
    return CCL_SUCCESS;
}


uint32_t cclCanMakeExtendedIdentifier(uint32_t identifier) {
    return identifier | 0x80000000;
}


uint32_t cclCanMakeStandardIdentifier(uint32_t identifier) {
    return identifier & 0x7FFFFFFF;
}


uint32_t cclCanValueOfIdentifier(uint32_t identifier) {
    return identifier & 0x7FFFFFFF;
}


int32_t cclCanIsExtendedIdentifier(uint32_t identifier) {
    return ((identifier & 0x80000000) == 0x80000000) ? 1 : 0;
}


int32_t cclCanIsStandardIdentifier(uint32_t identifier) {
    return ((identifier & 0x80000000) == 0x00000000) ? 1 : 0;
}


int32_t cclCanGetChannelNumber(const char *networkName, int32_t *channel) {
    if (!gHasBusNames) {
        return CCL_NOTIMPLEMENTED;
    }

    if (networkName == nullptr || strlen(networkName) == 0) {
        return CCL_INVALIDNAME;
    }

    for (int32_t i = 1; i <= cMaxChannel; i++) {
        if (gCanBusContext[i].mBusName == networkName) {
            if (channel != nullptr) {
                *channel = i;
            }
            return CCL_SUCCESS;
        }
    }

    return CCL_NETWORKNOTDEFINED; // There is no CAN network with the given name.
}

#pragma endregion

#pragma region LIN Frames

// ============================================================================
// LIN
// ============================================================================

int32_t cclLinSetPreDispatchMessageHandler(int32_t channel,
                                           uint32_t identifier,
                                           void(*function)(struct cclLinPreDispatchMessage *frame)) {
    CCL_STATECHECK(eInitMeasurement)

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    if (function == nullptr) {
        return CCL_INVALIDFUNCTIONPOINTER;
    }

    uint8 requestType = (identifier == CCL_LIN_ALLMESSAGES) ? kVIA_AllId : kVIA_OneId;

    VLinPreDispatchMessage *request = new VLinPreDispatchMessage;
    request->mCallbackFunction = function;
    request->mContext = &gLinBusContext[channel];
    request->mHandle = nullptr;

    VIAResult rc = gLinBusContext[channel].mBus->CreatePreDispatchMessageRequest(&(request->mHandle), request,
                                                                                 requestType, identifier, channel);
    if (rc != kVIA_OK) {
        delete request;
        if (rc == kVIA_ServiceNotRunning) {
            return CCL_WRONGSTATE;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    gModule->mLinPreDispatchMessage.push_back(request);
    return CCL_SUCCESS;
}


int32_t cclLinSetSleepModeEventHandler(int32_t channel,
                                       void(*function)(struct cclLinSleepModeEvent *event)) {
    CCL_STATECHECK(eInitMeasurement)

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    if (function == nullptr) {
        return CCL_INVALIDFUNCTIONPOINTER;
    }

    VLinSleepModeEvent *request = new VLinSleepModeEvent;
    request->mCallbackFunction = function;
    request->mContext = &gLinBusContext[channel];
    request->mHandle = nullptr;

    VIAResult rc = gLinBusContext[channel].mBus->CreateSleepModeEventRequest(&(request->mHandle), request, channel);
    if (rc != kVIA_OK) {
        delete request;
        if (rc == kVIA_ServiceNotRunning) {
            return CCL_WRONGSTATE;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    gModule->mLinSleepModeEvent.push_back(request);
    return CCL_SUCCESS;
}


int32_t cclLinSetWakeupFrameHandler(int32_t channel,
                                    void(*function)(struct cclLinWakeupFrame *frame)) {
    CCL_STATECHECK(eInitMeasurement)

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    if (function == nullptr) {
        return CCL_INVALIDFUNCTIONPOINTER;
    }

    VLinWakeupFrame *request = new VLinWakeupFrame;
    request->mCallbackFunction = function;
    request->mContext = &gLinBusContext[channel];
    request->mHandle = nullptr;

    VIAResult rc = gLinBusContext[channel].mBus->CreateWakeupFrameRequest(&(request->mHandle), request, channel);
    if (rc != kVIA_OK) {
        delete request;
        if (rc == kVIA_ServiceNotRunning) {
            return CCL_WRONGSTATE;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    gModule->mLinWakeupFrame.push_back(request);
    return CCL_SUCCESS;
}


int32_t cclLinSetSlaveResponseChangedHandler(int32_t channel,
                                             uint32_t identifier,
                                             void(*function)(struct cclLinSlaveResponse *frame)) {
    CCL_STATECHECK(eInitMeasurement)

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    if (function == nullptr) {
        return CCL_INVALIDFUNCTIONPOINTER;
    }

    uint8 requestType = (identifier == CCL_LIN_ALLMESSAGES) ? kVIA_AllId : kVIA_OneId;

    VLinSlaveResponseChange *request = new VLinSlaveResponseChange;
    request->mCallbackFunction = function;
    request->mContext = &gLinBusContext[channel];
    request->mHandle = nullptr;

    VIAResult rc = gLinBusContext[channel].mBus->CreateSlaveResponseChangedRequest(&(request->mHandle), request,
                                                                                   requestType, identifier, channel);
    if (rc != kVIA_OK) {
        delete request;
        if (rc == kVIA_ServiceNotRunning) {
            return CCL_WRONGSTATE;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    gModule->mLinSlaveResponseChange.push_back(request);
    return CCL_SUCCESS;
}


int32_t cclLinSetFrameHandler(int32_t channel,
                              uint32_t identifier,
                              void (*function)(struct cclLinFrame *frame)) {
    CCL_STATECHECK(eInitMeasurement)

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    if (function == nullptr) {
        return CCL_INVALIDFUNCTIONPOINTER;
    }

    uint8 requestType = (identifier == CCL_LIN_ALLMESSAGES) ? kVIA_AllId : kVIA_OneId;

    VLinMessageRequest *request = new VLinMessageRequest;
    request->mCallbackFunction = function;
    request->mContext = &gLinBusContext[channel];
    request->mHandle = nullptr;

    VIAResult rc = gLinBusContext[channel].mBus->CreateMessageRequest2(&(request->mHandle), request, requestType,
                                                                       identifier, channel);
    if (rc != kVIA_OK) {
        delete request;
        if (rc == kVIA_ServiceNotRunning) {
            return CCL_WRONGSTATE;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    gModule->mLinMessageRequests.push_back(request);
    return CCL_SUCCESS;
}

int32_t cclLinSetErrorHandler(int32_t channel, uint32_t identifier, void(*function)(struct cclLinError *error)) {
    CCL_STATECHECK(eInitMeasurement)

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    if (function == nullptr) {
        return CCL_INVALIDFUNCTIONPOINTER;
    }

    uint8 requestType = (identifier == CCL_LIN_ALLMESSAGES) ? kVIA_AllId : kVIA_OneId;

    VLinErrorRequest *request = new VLinErrorRequest;
    request->mCallbackFunction = function;
    request->mContext = &gLinBusContext[channel];
    for (size_t i = 0; i < VLinErrorRequest::kMaxHandles; ++i) {
        request->mHandle[i] = nullptr;
    }

    VIALin *bus = gLinBusContext[channel].mBus;
    int cnt = 0;

    VIAResult rc = bus->CreateCheckSumErrorRequest(&(request->mHandle[0]), request, requestType, identifier, channel);
    if (rc == kVIA_OK) cnt++;
    rc = bus->CreateTransmissionErrorRequest(&(request->mHandle[1]), request, requestType, identifier, channel);
    if (rc == kVIA_OK) cnt++;
    rc = bus->CreateReceiveErrorRequest(&(request->mHandle[2]), request, channel);
    if (rc == kVIA_OK) cnt++;
    rc = bus->CreateSyncErrorRequest(&(request->mHandle[3]), request, channel);
    if (rc == kVIA_OK) cnt++;
    rc = bus->CreateSlaveTimeoutRequest(&(request->mHandle[4]), request, requestType, identifier, channel);
    if (rc == kVIA_OK) cnt++;
    if (cnt == 0) {
        // None of the above calls was successful:
        delete request;
        if (rc == kVIA_ServiceNotRunning) {
            return CCL_WRONGSTATE;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    gModule->mLinErrorRequests.push_back(request);
    return CCL_SUCCESS;
}

int32_t cclLinUpdateResponseData(int32_t channel,
                                 uint32_t id,
                                 uint8_t dlc,
                                 uint8_t data[8]) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement);

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    uint32 flags = kVIA_ReconfigureData;
    int32 response = -2;
    VIAResult rc = gLinBusContext[channel].mBus->OutputMessage(channel, id, flags, dlc, data, response);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


extern int32_t cclLinSendHeader(int32_t channel, uint32_t id) {
    CCL_STATECHECK(eRunMeasurement);

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    uint32 flags = kVIA_ApplyHeader;
    int32 response = 0;
    uint8 dlc = 0;
    uint8 data[8];
    VIAResult rc = gLinBusContext[channel].mBus->OutputMessage(channel, id, flags, dlc, data, response);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclLinStartScheduler(int32_t channel) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement);

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    VIAResult rc = gLinBusContext[channel].mBus->LINStartScheduler(channel);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclLinStopScheduler(int32_t channel) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement);

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    VIAResult rc = gLinBusContext[channel].mBus->LINStopScheduler(channel);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclLinChangeSchedtable(int32_t channel, uint32_t tableIndex) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement);

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    VIAResult rc = gLinBusContext[channel].mBus->LINChangeSchedtableEx(channel, tableIndex);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclLinGetChannelNumber(const char *networkName, int32_t *channel) {
    if (!gHasBusNames) {
        return CCL_NOTIMPLEMENTED;
    }

    if (networkName == nullptr || strlen(networkName) == 0) {
        return CCL_INVALIDNAME;
    }

    for (int32_t i = 1; i <= cMaxChannel; i++) {
        if (gLinBusContext[i].mBusName == networkName) {
            if (channel != nullptr) {
                *channel = i;
            }
            return CCL_SUCCESS;
        }
    }

    return CCL_NETWORKNOTDEFINED; // There is no LIN network with the given name.
}


int32_t cclLinGotoSleep(int32_t channel) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement);

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    VIAResult rc = gLinBusContext[channel].mBus->LINGotoSleep(channel);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclLinGotoSleepInternal(int32_t channel) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement);

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    VIAResult rc = gLinBusContext[channel].mBus->LINGotoSleepInternal(channel);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclLinWakeup(int32_t channel) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement);

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    VIAResult rc = gLinBusContext[channel].mBus->LINWakeup(channel);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclLinWakeupInternal(int32_t channel) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement);

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    VIAResult rc = gLinBusContext[channel].mBus->LINWakeupInternal(channel);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclLinSetWakeupCondition(int32_t channel, uint32_t numWakeupSignals, uint32_t forgetWupsAfterMS) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement);

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    VIAResult rc = gLinBusContext[channel].mBus->LINSetWakeupCondition(channel, numWakeupSignals, forgetWupsAfterMS);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclLinSetWakeupTimings(int32_t channel, uint32_t ttobrk, uint32_t wakeupSignalCount, uint32_t widthinMicSec) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement);

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    VIAResult rc = gLinBusContext[channel].mBus->LINSetWakeupTimings(channel, ttobrk, wakeupSignalCount, widthinMicSec);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t
cclLinSetWakeupBehavior(int32_t channel, uint32_t restartScheduler, uint32_t wakeupIdentifier, uint32_t sendShortHeader,
                        uint32_t wakeupDelimiterLength) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement);

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gLinBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    VIAResult rc = gLinBusContext[channel].mBus->LINSetWakeupBehavior(channel, restartScheduler, wakeupIdentifier,
                                                                      sendShortHeader, wakeupDelimiterLength);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}

#pragma endregion

#pragma region Function Bus Value Entities

cclValueID cclValueGetID(cclDomain domain, const char *path, const char *member, cclValueRepresentation repr) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    switch (domain) {
        case CCL_COMMUNICATION_OBJECTS:
            return VPortValueEntity::CreateValueEntity(path, member, repr);
        case CCL_DISTRIBUTED_OBJECTS:
            return VDOValueEntity::CreateValueEntity(path, member, repr);
        default:
            return CCL_INVALIDDOMAIN;
    }
}

int32_t cclValueGetInteger(cclValueID valueID, int64_t *x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->GetInteger(x);
}

int32_t cclValueSetInteger(cclValueID valueID, int64_t x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->SetInteger(x);
}

int32_t cclValueGetFloat(cclValueID valueID, double *x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->GetFloat(x);
}

int32_t cclValueSetFloat(cclValueID valueID, double x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->SetFloat(x);
}

int32_t cclValueGetString(cclValueID valueID, char *buffer, int32_t bufferSize) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->GetString(buffer, bufferSize);
}

int32_t cclValueSetString(cclValueID valueID, const char *str) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->SetString(str);
}

int32_t cclValueGetData(cclValueID valueID, uint8_t *buffer, int32_t *bufferSize) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->GetData(buffer, bufferSize);
}

int32_t cclValueSetData(cclValueID valueID, const uint8_t *data, int32_t size) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->SetData(data, size);
}

int32_t cclValueGetEnum(cclValueID valueID, int32_t *x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->GetEnum(x);
}

int32_t cclValueSetEnum(cclValueID valueID, int32_t x) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->SetEnum(x);
}

int32_t cclValueGetUnionSelector(cclValueID valueID, int32_t *selector) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->GetUnionSelector(selector);
}

int32_t cclValueGetArraySize(cclValueID valueID, int32_t *size) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->GetArraySize(size);
}

int32_t cclValueSetArraySize(cclValueID valueID, int32_t size) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    if (size < 0)
        return CCL_PARAMETERINVALID;

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->SetArraySize(size);
}

int32_t cclValueGetType(cclValueID valueID, cclValueType *valueType) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->GetValueType(valueType);
}

int32_t cclValueIsValid(cclValueID valueID, bool *isValid) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->IsValid(isValid);
}

int32_t cclValueClear(cclValueID valueID) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->ClearValue();
}

int32_t cclValueGetUpdateTimeNS(cclValueID valueID, cclTime *time) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->GetUpdateTimeNS(time);
}

int32_t cclValueGetChangeTimeNS(cclValueID valueID, cclTime *time) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->GetChangeTimeNS(time);
}

int32_t cclValueGetState(cclValueID valueID, cclValueState *state) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->GetState(state);
}

int32_t cclValueSetHandler(cclValueID valueID, bool onUpdate, cclValueHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IValue *valueEntity = nullptr;
    if (IValue::LookupValue(valueID, valueEntity) != CCL_SUCCESS)
        return CCL_INVALIDVALUEID;

    return valueEntity->SetHandler(onUpdate, handler);
}

#pragma endregion

#pragma region Function Bus Functions and Call Contexts

cclFunctionID cclFunctionGetID(cclDomain domain, const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    switch (domain) {
        case CCL_COMMUNICATION_OBJECTS: {
            bool isClient = ::strstr(path, ".consumerSide") != nullptr;

            VIAFbViaService *fbAPI = GetFunctionBusAPI();
            if (isClient)
                return VClientFunction::CreateClientFunction(path);
            else
                return VServerFunction::CreateServerFunction(path);
        }
        case CCL_DISTRIBUTED_OBJECTS:
            return DOMethods::CreateDOMethod(path);
        default:
            return CCL_INVALIDDOMAIN;
    }
}

int32_t cclFunctionSetHandler(cclFunctionID functionID, cclCallState callState, cclCallHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    IFunctionPort *function = nullptr;
    if (VFunctionUtils::LookupFunction(functionID, function) != CCL_SUCCESS)
        return CCL_INVALIDFUNCTIONID;

    VIAFbFunctionCallState mappedState;
    int32_t callbackIndex = VFunctionUtils::MapCallState(callState, mappedState);
    if ((callbackIndex < 0) || (callbackIndex > 3))
        return CCL_PARAMETERINVALID;

    return function->SetHandler(callbackIndex, mappedState, handler);
}

cclCallContextID cclCreateCallContext(cclFunctionID functionID) {
    CCL_STATECHECK(eRunMeasurement);

    IFunctionPort *function = nullptr;
    if (VFunctionUtils::LookupFunction(functionID, function) != CCL_SUCCESS)
        return CCL_INVALIDFUNCTIONID;

    if (function->GetSide() != cclSide::CCL_SIDE_CONSUMER)
        return CCL_INVALIDSIDE;

    return function->BeginCall();
}

cclCallContextID cclCallContextMakePermanent(cclCallContextID ccID) {
    CCL_STATECHECK(eRunMeasurement);

    VCallContext *cc = gModule->mCallContexts.Get(ccID);
    if (cc == nullptr)
        return CCL_INVALIDCALLCONTEXTID;

    IFunctionPort *function = nullptr;
    if (VFunctionUtils::LookupFunction(cc->mFunctionID, function) != CCL_SUCCESS)
        return CCL_INTERNALERROR;

    VIAFbCallContext *ctxt;
    if (cc->mContext->MakePermanent(&ctxt) != kVIA_OK)
        return CCL_INTERNALERROR;

    return VFunctionUtils::CreateCallContext(ctxt, function->GetFunctionID(), true);
}

int32_t cclCallContextRelease(cclCallContextID ccID) {
    CCL_STATECHECK(eRunMeasurement | eStopMeasurement);

    return VFunctionUtils::DestroyCallContext(ccID) ? CCL_SUCCESS : CCL_INVALIDCALLCONTEXTID;
}

int32_t cclCallAsync(cclCallContextID ccID, cclCallHandler resultHandler) {
    CCL_STATECHECK(eRunMeasurement);

    VCallContext *cc = gModule->mCallContexts.Get(ccID);
    if (cc == nullptr)
        return CCL_INVALIDCALLCONTEXTID;

    IFunctionPort *function = nullptr;
    if (VFunctionUtils::LookupFunction(cc->mFunctionID, function) != CCL_SUCCESS)
        return CCL_INTERNALERROR;

    if (function->GetSide() != cclSide::CCL_SIDE_CONSUMER)
        return CCL_INVALIDSIDE;

    VIAFbFunctionCallState state;
    if (cc->mContext->GetCallState(&state) != CCL_SUCCESS)
        return CCL_INTERNALERROR;

    if (state != VIAFbFunctionCallState::eCalling)
        return CCL_INVALIDCALLCONTEXTSTATE;

    cc->mResultCallbackFunction = resultHandler;
    return function->InvokeCall(cc);
}

int32_t cclCallSync(cclCallContextID ccID, cclCallHandler resultHandler) {
    CCL_STATECHECK(eRunMeasurement);

    VCallContext *cc = gModule->mCallContexts.Get(ccID);
    if (cc == nullptr)
        return CCL_INVALIDCALLCONTEXTID;

    IFunctionPort *function = nullptr;
    if (VFunctionUtils::LookupFunction(cc->mFunctionID, function) != CCL_SUCCESS)
        return CCL_INTERNALERROR;

    if (function->GetSide() != cclSide::CCL_SIDE_CONSUMER)
        return CCL_INVALIDSIDE;

    VIAFbFunctionCallState state;
    if (cc->mContext->GetCallState(&state) != CCL_SUCCESS)
        return CCL_INTERNALERROR;

    if (state != VIAFbFunctionCallState::eCalling)
        return CCL_INVALIDCALLCONTEXTSTATE;

    cc->mResultCallbackFunction = resultHandler;
    return function->InvokeSynchronousCall(cc);
}

cclValueID cclCallContextValueGetID(cclCallContextID ccID, const char *member, cclValueRepresentation repr) {
    CCL_STATECHECK(eRunMeasurement);

    VCallContext *cc = gModule->mCallContexts.Get(ccID);
    if (cc == nullptr)
        return CCL_INVALIDCALLCONTEXTID;

    cclValueID valueID = VCallContextValue::CreateCallContextValue(cc->mContextID, member, repr);
    if (valueID >= 0) cc->mContextValues.push_back(valueID);
    return valueID;
}

int32_t cclCallContextReturn(cclCallContextID ccID, cclTime timeOffset) {
    CCL_STATECHECK(eRunMeasurement);

    VCallContext *cc = gModule->mCallContexts.Get(ccID);
    if (cc == nullptr)
        return CCL_INVALIDCALLCONTEXTID;

    VIAFbFunctionCallState callState;
    if (cc->mContext->GetCallState(&callState) != kVIA_OK)
        return CCL_INTERNALERROR;

    if ((callState != VIAFbFunctionCallState::eCalled) && (callState != VIAFbFunctionCallState::eCalling))
        return CCL_INVALIDCALLCONTEXTSTATE;

    VIAFbFunctionCallSide side;
    if (cc->mContext->GetSide(&side) != kVIA_OK)
        return CCL_INTERNALERROR;

    if (side != VIAFbFunctionCallSide::eServer)
        return CCL_INVALIDSIDE;

    if ((timeOffset < 0) && (timeOffset != CCL_TIME_OFFSET_NEVER))
        return CCL_INVALIDTIME;

    if (cc->IsSynchronousCall())
        return CCL_INVALIDCALLCONTEXTSTATE;

    if (cc->mContext->SetTimeToReply(timeOffset) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclCallContextSetDefaultAnswer(cclCallContextID ccID) {
    CCL_STATECHECK(eRunMeasurement);

    VCallContext *cc = gModule->mCallContexts.Get(ccID);
    if (cc == nullptr)
        return CCL_INVALIDCALLCONTEXTID;

    VIAFbFunctionCallState callState;
    if (cc->mContext->GetCallState(&callState) != kVIA_OK)
        return CCL_INTERNALERROR;

    if ((callState != VIAFbFunctionCallState::eCalled) && (callState != VIAFbFunctionCallState::eCalling))
        return CCL_INVALIDCALLCONTEXTSTATE;

    VIAFbFunctionCallSide side;
    if (cc->mContext->GetSide(&side) != kVIA_OK)
        return CCL_INTERNALERROR;

    if (side != VIAFbFunctionCallSide::eServer)
        return CCL_INVALIDSIDE;

    if (cc->mContext->SetDefaultAnswer() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclCallContextGetFunctionID(cclCallContextID ccID, cclFunctionID *functionID) {
    CCL_STATECHECK(eRunMeasurement);

    VCallContext *cc = gModule->mCallContexts.Get(ccID);
    if (cc == nullptr)
        return CCL_INVALIDCALLCONTEXTID;

    *functionID = cc->mFunctionID;
    return CCL_SUCCESS;
}

int32_t cclCallContextGetFunctionName(cclCallContextID ccID, char *buffer, int32_t bufferSize) {
    CCL_STATECHECK(eRunMeasurement);

    VCallContext *cc = gModule->mCallContexts.Get(ccID);
    if (cc == nullptr)
        return CCL_INVALIDCALLCONTEXTID;

    IFunctionPort *function = nullptr;
    if (VFunctionUtils::LookupFunction(cc->mFunctionID, function) != CCL_SUCCESS)
        return CCL_INTERNALERROR;

    size_t start = std::string::npos;
    size_t len = 0;

    const std::string &path = function->GetPath();

    if (path.back() == ']') {
        // free function "ValueEntities::TheFunction.providerSide[Client1,Server1]";
        start = path.rfind(':');
        if (start == std::string::npos)
            start = 0; // no namespace
        else
            ++start;

        size_t end = path.find('.', start);
        if (end == std::string::npos)
            return CCL_INTERNALERROR;

        len = end - start;
    } else {
        // service function "ValueEntities::TheService.providerSide[Client1,Server1].ServiceFunction";
        start = path.rfind('.');
        if (start == std::string::npos)
            return CCL_INTERNALERROR;
        ++start;

        len = path.length() - start;
    }

    if (bufferSize <= (int32_t) len) {
        return CCL_BUFFERTOSMALL;
    }

    UtilStringCopyN(buffer, bufferSize, path.c_str() + start, len);

    return CCL_SUCCESS;
}

int32_t cclCallContextGetFunctionPath(cclCallContextID ccID, char *buffer, int32_t bufferSize) {
    CCL_STATECHECK(eRunMeasurement);

    VCallContext *cc = gModule->mCallContexts.Get(ccID);
    if (cc == nullptr)
        return CCL_INVALIDCALLCONTEXTID;

    IFunctionPort *function = nullptr;
    if (VFunctionUtils::LookupFunction(cc->mFunctionID, function) != CCL_SUCCESS)
        return CCL_INTERNALERROR;

    const std::string &path = function->GetPath();

    size_t len = path.length();
    if (bufferSize <= (int32_t) len + 1)
        return CCL_BUFFERTOSMALL;

    UtilStringCopy(buffer, bufferSize, path.c_str());

    return CCL_SUCCESS;
}

int32_t cclCallContextIsSynchronousCall(cclCallContextID ccID, bool *isSynchronous) {
    if (isSynchronous == nullptr)
        return CCL_PARAMETERINVALID;
    VCallContext *cc = gModule->mCallContexts.Get(ccID);
    if (cc == nullptr)
        return CCL_INVALIDCALLCONTEXTID;
    *isSynchronous = cc->IsSynchronousCall();
    return CCL_SUCCESS;
}

#pragma endregion

#pragma region Services

cclServiceID cclServiceGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VService::CreateService(path);
}

int32_t cclServiceSetSimulator(cclProvidedServiceID providedServiceID, cclCallHandler simulator) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VProvidedService *providedService;
    int32_t result = VProvidedService::LookupProvidedService(providedServiceID, providedService);
    if (result != CCL_SUCCESS)
        return result;

    return providedService->SetSimulator(simulator);
}

cclConsumerID cclAddConsumerByName(cclServiceID serviceID, const char *name, bool isSimulated) {
    CCL_STATECHECK(eRunMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbServiceConsumer *consumer;
    VIAFbFEPState fepState = isSimulated ? VIAFbFEPState::eSimulated : VIAFbFEPState::eReal;
    if (service->mService->AddConsumerByName(name, fepState, &consumer) != kVIA_OK)
        return CCL_CANNOTADDDYNAMICCONSUMER;

    return VConsumer::CreateConsumer(consumer, true);
}

cclProviderID cclAddProviderByName(cclServiceID serviceID, const char *name, bool isSimulated) {
    CCL_STATECHECK(eRunMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbServiceProvider *provider;
    VIAFbFEPState fepState = isSimulated ? VIAFbFEPState::eSimulated : VIAFbFEPState::eReal;
    if (service->mService->AddProviderByName(name, fepState, &provider) != kVIA_OK)
        return CCL_CANNOTADDDYNAMICPROVIDER;

    return VProvider::CreateProvider(provider, true);
}

cclConsumerID cclAddConsumerByAddress(cclServiceID serviceID, cclAddressID addressID) {
    CCL_STATECHECK(eRunMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    VAddress *address;
    result = VAddress::LookupAddress(addressID, address);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbServiceConsumer *consumer;
    if (service->mService->AddConsumerByAddress(address->mAddress, &consumer) != kVIA_OK)
        return CCL_CANNOTADDDYNAMICCONSUMER;

    return VConsumer::CreateConsumer(consumer, true);
}

cclProviderID cclAddProviderByAddress(cclServiceID serviceID, cclAddressID addressID) {
    CCL_STATECHECK(eRunMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    VAddress *address;
    result = VAddress::LookupAddress(addressID, address);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbServiceProvider *provider;
    if (service->mService->AddProviderByAddress(address->mAddress, &provider) != kVIA_OK)
        return CCL_CANNOTADDDYNAMICPROVIDER;

    return VProvider::CreateProvider(provider, true);
}

int32_t cclRemoveConsumer(cclServiceID serviceID, cclConsumerID consumerID) {
    CCL_STATECHECK(eRunMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    VConsumer *consumer;
    result = VConsumer::LookupConsumer(consumerID, consumer);
    if (result != CCL_SUCCESS)
        return result;

    if (service->mService->RemoveConsumer(consumer->mPort) != kVIA_OK)
        return CCL_STATICCONSUMER;

    // RemoveConsumer callback implicitly releases the VIA Object and C API object
    return CCL_SUCCESS;
}

int32_t cclRemoveProvider(cclServiceID serviceID, cclProviderID providerID) {
    CCL_STATECHECK(eRunMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    VProvider *provider;
    result = VProvider::LookupProvider(providerID, provider);
    if (result != CCL_SUCCESS)
        return result;

    if (service->mService->RemoveProvider(provider->mPort) != kVIA_OK)
        return CCL_STATICPROVIDER;

    // RemoveProvider callback implicitly releases the VIA Object and C API object
    return CCL_SUCCESS;
}

cclConsumerID cclGetConsumer(cclServiceID serviceID, int32_t bindingID) {
    CCL_STATECHECK(eRunMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbServiceConsumer *consumer;
    if (service->mService->GetConsumer(bindingID, &consumer) != kVIA_OK)
        return CCL_CONSUMERNOTFOUND;

    return VConsumer::CreateConsumer(consumer, true);
}

cclProviderID cclGetProvider(cclServiceID serviceID, int32_t bindingID) {
    CCL_STATECHECK(eRunMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbServiceProvider *provider;
    if (service->mService->GetProvider(bindingID, &provider) != kVIA_OK)
        return CCL_PROVIDERNOTFOUND;

    return VProvider::CreateProvider(provider, true);
}

cclConsumedServiceID cclGetConsumedService(cclServiceID serviceID, int32_t consumerID, int32_t providerID) {
    CCL_STATECHECK(eRunMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbServiceConsumer *consumer;
    if (service->mService->GetConsumer(consumerID, &consumer) != kVIA_OK)
        return CCL_CONSUMERNOTFOUND;

    VIAFbServiceProvider *provider;
    if (service->mService->GetProvider(providerID, &provider) != kVIA_OK)
        return CCL_PROVIDERNOTFOUND;

    VIAFbConsumedService *consumedService;
    if (service->mService->GetConsumedService(consumerID, providerID, &consumedService) != kVIA_OK)
        return CCL_INTERNALERROR;

    return VConsumedService::CreateConsumedService(consumedService);
}

cclProvidedServiceID cclGetProvidedService(cclServiceID serviceID, int32_t consumerID, int32_t providerID) {
    CCL_STATECHECK(eRunMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbServiceConsumer *consumer;
    if (service->mService->GetConsumer(consumerID, &consumer) != kVIA_OK)
        return CCL_CONSUMERNOTFOUND;

    VIAFbServiceProvider *provider;
    if (service->mService->GetProvider(providerID, &provider) != kVIA_OK)
        return CCL_PROVIDERNOTFOUND;

    VIAFbProvidedService *providedService;
    if (service->mService->GetProvidedService(consumerID, providerID, &providedService) != kVIA_OK)
        return CCL_INTERNALERROR;

    return VProvidedService::CreateProvidedService(providedService);
}

cclConsumerID cclConsumerGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VConsumer::CreateConsumer(path);
}

cclProviderID cclProviderGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VProvider::CreateProvider(path);
}

int32_t cclProvideService(cclProviderID providerID) {
    CCL_STATECHECK(eRunMeasurement);

    VProvider *provider;
    int32_t result = VProvider::LookupProvider(providerID, provider);
    if (result != CCL_SUCCESS)
        return result;

    if (provider->mPort->ProvideService() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclReleaseService(cclProviderID providerID) {
    CCL_STATECHECK(eRunMeasurement);

    VProvider *provider;
    int32_t result = VProvider::LookupProvider(providerID, provider);
    if (result != CCL_SUCCESS)
        return result;

    if (provider->mPort->ReleaseService() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclIsServiceProvided(cclProviderID providerID, bool *isProvided) {
    CCL_STATECHECK(eRunMeasurement);

    if (isProvided == nullptr)
        return CCL_PARAMETERINVALID;

    VProvider *provider;
    int32_t result = VProvider::LookupProvider(providerID, provider);
    if (result != CCL_SUCCESS)
        return result;

    if (provider->mPort->IsServiceProvided(isProvided) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclConsumerGetBindingID(cclConsumerID consumerID, int32_t *bindingID) {
    CCL_STATECHECK(eRunMeasurement);

    if (bindingID == nullptr)
        return CCL_PARAMETERINVALID;

    VConsumer *consumer;
    int32_t result = VConsumer::LookupConsumer(consumerID, consumer);
    if (result != CCL_SUCCESS)
        return result;

    if (consumer->mPort->GetBindingId(bindingID) != kVIA_OK)
        return CCL_CONSUMERNOTBOUND;

    return CCL_SUCCESS;
}

int32_t cclProviderGetBindingID(cclProviderID providerID, int32_t *bindingID) {
    CCL_STATECHECK(eRunMeasurement);

    if (bindingID == nullptr)
        return CCL_PARAMETERINVALID;

    VProvider *provider;
    int32_t result = VProvider::LookupProvider(providerID, provider);
    if (result != CCL_SUCCESS)
        return result;

    if (provider->mPort->GetBindingId(bindingID) != kVIA_OK)
        return CCL_PROVIDERNOTBOUND;

    return CCL_SUCCESS;
}

cclConsumedServiceID cclConsumedServiceGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VConsumedService::CreateConsumedService(path);
}

cclProvidedServiceID cclProvidedServiceGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VProvidedService::CreateProvidedService(path);
}

cclValueID
cclConsumedServiceValueGetID(cclConsumedServiceID consumedServiceID, const char *member, cclValueRepresentation repr) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VConsumedService *consumedService;
    int32_t result = VConsumedService::LookupConsumedService(consumedServiceID, consumedService);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbValuePort *valuePort;
    if (consumedService->mPort->GetValuePort(&valuePort) != kVIA_OK)
        return CCL_INTERNALERROR;

    return VPortValueEntity::CreateValueEntity(valuePort, member, repr);
}

cclValueID
cclProvidedServiceValueGetID(cclProvidedServiceID providedServiceID, const char *member, cclValueRepresentation repr) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VProvidedService *providedService;
    int32_t result = VProvidedService::LookupProvidedService(providedServiceID, providedService);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbValuePort *valuePort;
    if (providedService->mPort->GetValuePort(&valuePort) != kVIA_OK)
        return CCL_INTERNALERROR;

    return VPortValueEntity::CreateValueEntity(valuePort, member, repr);
}

int32_t cclRequestService(cclConsumedServiceID consumedServiceID) {
    CCL_STATECHECK(eRunMeasurement);

    VConsumedService *consumedService;
    int32_t result = VConsumedService::LookupConsumedService(consumedServiceID, consumedService);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedService->mPort->RequestService() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclReleaseServiceRequest(cclConsumedServiceID consumedServiceID) {
    CCL_STATECHECK(eRunMeasurement);

    VConsumedService *consumedService;
    int32_t result = VConsumedService::LookupConsumedService(consumedServiceID, consumedService);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedService->mPort->ReleaseService() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclIsServiceRequested(cclConsumedServiceID consumedServiceID, bool *isRequested) {
    CCL_STATECHECK(eRunMeasurement);

    if (isRequested == nullptr)
        return CCL_PARAMETERINVALID;

    VConsumedService *consumedService;
    int32_t result = VConsumedService::LookupConsumedService(consumedServiceID, consumedService);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedService->mPort->IsServiceRequested(isRequested) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclDiscoverProviders(cclConsumerID consumerID) {
    CCL_STATECHECK(eRunMeasurement);

    VConsumer *consumer;
    int32_t result = VConsumer::LookupConsumer(consumerID, consumer);
    if (result != CCL_SUCCESS)
        return result;

    if (consumer->mPort->DiscoverProviders() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAnnounceProvider(cclProviderID providerID) {
    CCL_STATECHECK(eRunMeasurement);

    VProvider *provider;
    int32_t result = VProvider::LookupProvider(providerID, provider);
    if (result != CCL_SUCCESS)
        return result;

    if (provider->mPort->AnnounceProvider() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclUnannounceProvider(cclProviderID providerID) {
    CCL_STATECHECK(eRunMeasurement);

    VProvider *provider;
    int32_t result = VProvider::LookupProvider(providerID, provider);
    if (result != CCL_SUCCESS)
        return result;

    if (provider->mPort->UnannounceProvider() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAnnounceProviderToConsumer(cclServiceID serviceID, cclProviderID providerID, cclConsumerID consumerID) {
    CCL_STATECHECK(eRunMeasurement);

    VProvider *provider;
    int32_t result = VProvider::LookupProvider(providerID, provider);
    if (result != CCL_SUCCESS)
        return result;

    VConsumer *consumer;
    result = VConsumer::LookupConsumer(consumerID, consumer);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbBindingId providerBindingID;
    VIAFbBindingId consumerBindingID;
    if (provider->mPort->GetBindingId(&providerBindingID) != kVIA_OK)
        return CCL_INVALIDBINDING;
    if (consumer->mPort->GetBindingId(&consumerBindingID) != kVIA_OK)
        return CCL_INVALIDBINDING;

    VService *service;
    result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbProvidedService *providedService;
    if (service->mService->GetProvidedService(consumerBindingID, providerBindingID, &providedService) != kVIA_OK)
        return CCL_INTERNALERROR;

    if (providedService->AnnounceProvider() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclSetConsumerAddress(cclConsumerID consumerID, cclAddressID addressID, cclProviderID providerID) {
    CCL_STATECHECK(eRunMeasurement);

    VConsumer *consumer;
    int32_t result = VConsumer::LookupConsumer(consumerID, consumer);
    if (result != CCL_SUCCESS)
        return result;

    VAddress *address;
    result = VAddress::LookupAddress(addressID, address);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbServiceProvider *fbProvider = nullptr;
    if (providerID != CCL_PROVIDER_NONE) {
        VProvider *provider;
        result = VProvider::LookupProvider(providerID, provider);
        if (result != CCL_SUCCESS)
            return result;

        fbProvider = provider->mPort;
    }

    if (consumer->mPort->SetAddress(address->mAddress, fbProvider) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclSetProviderAddress(cclProviderID providerID, cclAddressID addressID, cclConsumerID consumerID) {
    CCL_STATECHECK(eRunMeasurement);

    VProvider *provider;
    int32_t result = VProvider::LookupProvider(providerID, provider);
    if (result != CCL_SUCCESS)
        return result;

    VAddress *address;
    result = VAddress::LookupAddress(addressID, address);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbServiceConsumer *fbConsumer = nullptr;
    if (consumerID != CCL_CONSUMER_NONE) {
        VConsumer *consumer;
        result = VConsumer::LookupConsumer(consumerID, consumer);
        if (result != CCL_SUCCESS)
            return result;

        fbConsumer = consumer->mPort;
    }

    if (provider->mPort->SetAddress(address->mAddress, fbConsumer) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclConnectAsyncConsumer(cclConsumedServiceID consumedServiceID, cclConnectionEstablishedHandler success,
                                cclConnectionFailedHandler failure) {
    CCL_STATECHECK(eRunMeasurement);

    VConsumedService *consumedService;
    int32_t result = VConsumedService::LookupConsumedService(consumedServiceID, consumedService);
    if (result != CCL_SUCCESS)
        return result;

    return consumedService->ConnectAsync(success, failure);
}

int32_t cclConnectAsyncProvider(cclProvidedServiceID providedServiceID, cclConnectionEstablishedHandler success,
                                cclConnectionFailedHandler failure) {
    CCL_STATECHECK(eRunMeasurement);

    VProvidedService *providedService;
    int32_t result = VProvidedService::LookupProvidedService(providedServiceID, providedService);
    if (result != CCL_SUCCESS)
        return result;

    return providedService->ConnectAsync(success, failure);
}

int32_t cclDisconnectConsumer(cclConsumedServiceID consumedServiceID) {
    CCL_STATECHECK(eRunMeasurement);

    VConsumedService *consumedService;
    int32_t result = VConsumedService::LookupConsumedService(consumedServiceID, consumedService);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedService->mPort->Disconnect() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclDisconnectProvider(cclProvidedServiceID providedServiceID) {
    CCL_STATECHECK(eRunMeasurement);

    VProvidedService *providedService;
    int32_t result = VProvidedService::LookupProvidedService(providedServiceID, providedService);
    if (result != CCL_SUCCESS)
        return result;

    if (providedService->mPort->Disconnect() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclSetConsumerDiscoveredHandler(cclProviderID providerID, cclConsumerDiscoveredHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VProvider *provider;
    int32_t result = VProvider::LookupProvider(providerID, provider);
    if (result != CCL_SUCCESS)
        return result;

    return provider->SetConsumerDiscoveredHandler(handler);
}

int32_t cclSetProviderDiscoveredHandler(cclConsumerID consumerID, cclProviderDiscoveredHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VConsumer *consumer;
    int32_t result = VConsumer::LookupConsumer(consumerID, consumer);
    if (result != CCL_SUCCESS)
        return result;

    return consumer->SetProviderDiscoveredHandler(handler);
}

int32_t cclSetConnectionRequestedHandler(cclProviderID providerID, cclConnectionRequestedHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VProvider *provider;
    int32_t result = VProvider::LookupProvider(providerID, provider);
    if (result != CCL_SUCCESS)
        return result;

    return provider->SetConnectionRequestedHandler(handler);
}

int32_t cclSetServiceDiscoveryHandler(cclServiceID serviceID, cclServiceDiscoveryHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    return service->SetServiceDiscoveryHandler(handler);
}

int32_t cclSetServiceConsumerDiscoveredHandler(cclServiceID serviceID, cclConsumerDiscoveredHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    return service->SetConsumerDiscoveredHandler(handler);
}

int32_t cclSetServiceProviderDiscoveredHandler(cclServiceID serviceID, cclProviderDiscoveredHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    return service->SetProviderDiscoveredHandler(handler);
}

#pragma endregion

#pragma region PDU Signal access

cclPDUSenderID cclPDUSenderGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VPDUSender::CreatePDUSender(path);
}

cclPDUReceiverID cclPDUReceiverGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VPDUReceiver::CreatePDUReceiver(path);
}

cclPDUProviderID cclPDUProviderGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VServicePDUProvider::CreatePDUProvider(path);
}

cclValueID cclPDUSenderSignalValueGetID(cclPDUSenderID pduSenderID, const char *signal, const char *member,
                                        cclValueRepresentation repr) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VPDUSender *pduSender;
    int32_t result = VPDUSender::LookupPDUSender(pduSenderID, pduSender);
    if (result != CCL_SUCCESS)
        return result;

    return VPDUSignalValue<VIAFbPDUSenderPort>::CreateSignalValue(pduSender->mPort, signal, member, repr);
}

cclValueID cclPDUReceiverSignalValueGetID(cclPDUReceiverID pduReceiverID, const char *signal, const char *member,
                                          cclValueRepresentation repr) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VPDUReceiver *pduReceiver;
    int32_t result = VPDUReceiver::LookupPDUReceiver(pduReceiverID, pduReceiver);
    if (result != CCL_SUCCESS)
        return result;

    return VPDUSignalValue<VIAFbPDUReceiverPort>::CreateSignalValue(pduReceiver->mPort, signal, member, repr);
}

cclValueID cclPDUProviderSignalValueGetID(cclPDUProviderID pduProviderID, const char *signal, const char *member,
                                          cclValueRepresentation repr) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VServicePDUProvider *pduProvider;
    int32_t result = VServicePDUProvider::LookupPDUProvider(pduProviderID, pduProvider);
    if (result != CCL_SUCCESS)
        return result;

    return VPDUSignalValue<VIAFbServicePDUProvider>::CreateSignalValue(pduProvider->mPort, signal, member, repr);
}

#pragma endregion

#pragma region General Service PDU Subscription

cclPDUReceiverID cclConsumedPDUGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VConsumedServicePDU::CreateConsumedPDU(path);
}

cclPDUSenderID cclProvidedPDUGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VProvidedServicePDU::CreateProvidedPDU(path);
}

int32_t cclPDUGetNrOfSubscribedConsumers(cclPDUProviderID pduProviderID, int32_t *nrOfConsumers) {
    CCL_STATECHECK(eRunMeasurement);

    VServicePDUProvider *pduProvider;
    int32_t result = VServicePDUProvider::LookupPDUProvider(pduProviderID, pduProvider);
    if (result != CCL_SUCCESS)
        return result;

    return pduProvider->GetNrOfSubscribedConsumers(nrOfConsumers);
}

int32_t
cclPDUGetSubscribedConsumers(cclPDUProviderID pduProviderID, cclConsumerID *consumerBuffer, int32_t *bufferSize) {
    CCL_STATECHECK(eRunMeasurement);

    VServicePDUProvider *pduProvider;
    int32_t result = VServicePDUProvider::LookupPDUProvider(pduProviderID, pduProvider);
    if (result != CCL_SUCCESS)
        return result;

    return pduProvider->GetSubscribedConsumers(consumerBuffer, bufferSize);
}

int32_t cclProvidedPDUSetSubscriptionStateIsolated(cclPDUSenderID pduSenderID, bool subscribed) {
    CCL_STATECHECK(eRunMeasurement);

    VProvidedServicePDU *providedPDU;
    int32_t result = VProvidedServicePDU::LookupProvidedPDU(pduSenderID, providedPDU);
    if (result != CCL_SUCCESS)
        return result;

    if (providedPDU->SetSubscriptionStateIsolated(subscribed) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclConsumedPDUSetSubscriptionStateIsolated(cclPDUReceiverID pduReceiverID, bool subscribed) {
    CCL_STATECHECK(eRunMeasurement);

    VConsumedServicePDU *consumedPDU;
    int32_t result = VConsumedServicePDU::LookupConsumedPDU(pduReceiverID, consumedPDU);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedPDU->SetSubscriptionStateIsolated(subscribed) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

#pragma endregion

#pragma region General Event Subscription

cclConsumedEventID cclConsumedEventGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VConsumedEvent::CreateConsumedEvent(path);
}

cclProvidedEventID cclProvidedEventGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VProvidedEvent::CreateProvidedEvent(path);
}

cclEventProviderID cclEventProviderGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VEventProvider::CreateEventProvider(path);
}

int32_t cclEventGetNrOfSubscribedConsumers(cclEventProviderID eventProviderID, int32_t *nrOfConsumers) {
    CCL_STATECHECK(eRunMeasurement);

    VEventProvider *eventProvider;
    int32_t result = VEventProvider::LookupEventProvider(eventProviderID, eventProvider);
    if (result != CCL_SUCCESS)
        return result;

    return eventProvider->GetNrOfSubscribedConsumers(nrOfConsumers);
}

int32_t
cclEventGetSubscribedConsumers(cclEventProviderID eventProviderID, cclConsumerID *consumerBuffer, int32_t *bufferSize) {
    CCL_STATECHECK(eRunMeasurement);

    VEventProvider *eventProvider;
    int32_t result = VEventProvider::LookupEventProvider(eventProviderID, eventProvider);
    if (result != CCL_SUCCESS)
        return result;

    return eventProvider->GetSubscribedConsumers(consumerBuffer, bufferSize);
}

int32_t cclConsumedEventSetSubscriptionStateIsolated(cclConsumedEventID consumedEventID, bool subscribed) {
    CCL_STATECHECK(eRunMeasurement);

    VConsumedEvent *consumedEvent;
    int32_t result = VConsumedEvent::LookupConsumedEvent(consumedEventID, consumedEvent);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedEvent->SetSubscriptionStateIsolated(subscribed) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclProvidedEventSetSubscriptionStateIsolated(cclProvidedEventID providedEventID, bool subscribed) {
    CCL_STATECHECK(eRunMeasurement);

    VProvidedEvent *providedEvent;
    int32_t result = VProvidedEvent::LookupProvidedEvent(providedEventID, providedEvent);
    if (result != CCL_SUCCESS)
        return result;

    if (providedEvent->SetSubscriptionStateIsolated(subscribed) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

#pragma endregion

#pragma region Service Field Access

cclConsumedFieldID cclConsumedFieldGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VConsumedField::CreateConsumedField(path);
}

cclProvidedFieldID cclProvidedFieldGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VProvidedField::CreateProvidedField(path);
}

cclFieldProviderID cclFieldProviderGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VFieldProvider::CreateFieldProvider(path);
}

int32_t cclFieldGetNrOfSubscribedConsumers(cclFieldProviderID fieldProviderID, int32_t *nrOfConsumers) {
    CCL_STATECHECK(eRunMeasurement);

    VFieldProvider *fieldProvider;
    int32_t result = VFieldProvider::LookupFieldProvider(fieldProviderID, fieldProvider);
    if (result != CCL_SUCCESS)
        return result;

    return fieldProvider->GetNrOfSubscribedConsumers(nrOfConsumers);
}

int32_t
cclFieldGetSubscribedConsumers(cclFieldProviderID fieldProviderID, cclConsumerID *consumerBuffer, int32_t *bufferSize) {
    CCL_STATECHECK(eRunMeasurement);

    VFieldProvider *fieldProvider;
    int32_t result = VFieldProvider::LookupFieldProvider(fieldProviderID, fieldProvider);
    if (result != CCL_SUCCESS)
        return result;

    return fieldProvider->GetSubscribedConsumers(consumerBuffer, bufferSize);
}

int32_t cclConsumedFieldSetSubscriptionStateIsolated(cclConsumedFieldID consumedFieldID, bool subscribed) {
    CCL_STATECHECK(eRunMeasurement);

    VConsumedField *consumedField;
    int32_t result = VConsumedField::LookupConsumedField(consumedFieldID, consumedField);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedField->SetSubscriptionStateIsolated(subscribed) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclProvidedFieldSetSubscriptionStateIsolated(cclProvidedFieldID providedFieldID, bool subscribed) {
    CCL_STATECHECK(eRunMeasurement);

    VProvidedField *providedField;
    int32_t result = VProvidedField::LookupProvidedField(providedFieldID, providedField);
    if (result != CCL_SUCCESS)
        return result;

    if (providedField->SetSubscriptionStateIsolated(subscribed) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

#pragma endregion

#pragma region Abstract Binding

cclAddressID cclAbstractCreateAddress(cclServiceID serviceID, const char *endPointName) {
    CCL_STATECHECK(eRunMeasurement);

    VService *service;
    int32_t result = VService::LookupService(serviceID, service);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbViaService *fbAPI = GetFunctionBusAPI();
    VIAFbAbstractBinding *abstractBinding;
    if (fbAPI->GetAbstractBinding(&abstractBinding) != kVIA_OK)
        return CCL_INTERNALERROR;

    NDetail::VIAObjectGuard <VIAFbAddressHandle> address;
    if (abstractBinding->CreateAddress(service->mService, endPointName, &(address.mVIAObject)) != kVIA_OK)
        return CCL_CANNOTCREATEADDRESS;

    return VAddress::CreateAddress(address.Release(), true);
}

int32_t cclAbstractGetBindingID(cclAddressID addressID, int32_t *bindingID) {
    CCL_STATECHECK(eRunMeasurement);

    VIAFbViaService *fbAPI = GetFunctionBusAPI();
    VIAFbAbstractBinding *abstractBinding;
    if (fbAPI->GetAbstractBinding(&abstractBinding) != kVIA_OK)
        return CCL_INTERNALERROR;

    VAddress *address;
    int32_t result = VAddress::LookupAddress(addressID, address);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbBindingId fbBindingID;
    if (abstractBinding->GetBindingId(address->mAddress, &fbBindingID) != kVIA_OK)
        return CCL_INVALIDBINDING;

    *bindingID = static_cast<int32_t>(fbBindingID);
    return CCL_SUCCESS;
}

int32_t cclAbstractGetDisplayName(cclAddressID addressID, char *buffer, int32_t bufferSize) {
    CCL_STATECHECK(eRunMeasurement);

    VIAFbViaService *fbAPI = GetFunctionBusAPI();
    VIAFbAbstractBinding *abstractBinding;
    if (fbAPI->GetAbstractBinding(&abstractBinding) != kVIA_OK)
        return CCL_INTERNALERROR;

    VAddress *address;
    int32_t result = VAddress::LookupAddress(addressID, address);
    if (result != CCL_SUCCESS)
        return result;

    result = abstractBinding->GetDisplayName(address->mAddress, buffer, bufferSize);
    if (result == kVIA_BufferToSmall)
        return CCL_BUFFERTOSMALL;

    return (result == kVIA_OK) ? CCL_SUCCESS : CCL_INTERNALERROR;
}

cclConsumedEventID cclAbstractConsumedEventGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VAbstractConsumedEvent::CreateConsumedEvent(path);
}

cclEventProviderID cclAbstractEventProviderGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VAbstractEventProvider::CreateEventProvider(path);
}

int32_t cclAbstractRequestEvent(cclConsumedEventID consumedEventID) {
    CCL_STATECHECK(eRunMeasurement);

    VAbstractConsumedEvent *consumedEvent;
    int32_t result = VAbstractConsumedEvent::LookupConsumedEvent(consumedEventID, consumedEvent);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedEvent->mAbstractConsumedEvent->RequestEvent() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAbstractReleaseEvent(cclConsumedEventID consumedEventID) {
    CCL_STATECHECK(eRunMeasurement);

    VAbstractConsumedEvent *consumedEvent;
    int32_t result = VAbstractConsumedEvent::LookupConsumedEvent(consumedEventID, consumedEvent);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedEvent->mAbstractConsumedEvent->ReleaseEvent() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAbstractIsEventRequested(cclConsumedEventID consumedEventID, bool *isRequested) {
    CCL_STATECHECK(eRunMeasurement);

    if (isRequested == nullptr)
        return CCL_PARAMETERINVALID;

    VAbstractConsumedEvent *consumedEvent;
    int32_t result = VAbstractConsumedEvent::LookupConsumedEvent(consumedEventID, consumedEvent);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedEvent->mAbstractConsumedEvent->IsEventRequested(isRequested) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAbstractSubscribeEvent(cclConsumedEventID consumedEventID) {
    CCL_STATECHECK(eRunMeasurement);

    VAbstractConsumedEvent *consumedEvent;
    int32_t result = VAbstractConsumedEvent::LookupConsumedEvent(consumedEventID, consumedEvent);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedEvent->mAbstractConsumedEvent->SubscribeEvent() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAbstractUnsubscribeEvent(cclConsumedEventID consumedEventID) {
    CCL_STATECHECK(eRunMeasurement);

    VAbstractConsumedEvent *consumedEvent;
    int32_t result = VAbstractConsumedEvent::LookupConsumedEvent(consumedEventID, consumedEvent);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedEvent->mAbstractConsumedEvent->UnsubscribeEvent() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t
cclAbstractSetEventSubscribedHandler(cclEventProviderID providerID, cclAbstractEventSubscriptionHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VAbstractEventProvider *eventProvider;
    int32_t result = VAbstractEventProvider::LookupEventProvider(providerID, eventProvider);
    if (result != CCL_SUCCESS)
        return result;

    if (eventProvider->SetEventSubscribedHandler(handler) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t
cclAbstractSetEventUnsubscribedHandler(cclEventProviderID providerID, cclAbstractEventSubscriptionHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VAbstractEventProvider *eventProvider;
    int32_t result = VAbstractEventProvider::LookupEventProvider(providerID, eventProvider);
    if (result != CCL_SUCCESS)
        return result;

    if (eventProvider->SetEventUnsubscribedHandler(handler) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

cclPDUReceiverID cclAbstractConsumedPDUGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VAbstractConsumedServicePDU::CreateConsumedPDU(path);
}

cclPDUProviderID cclAbstractPDUProviderGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VAbstractServicePDUProvider::CreatePDUProvider(path);
}

int32_t cclAbstractRequestPDU(cclPDUReceiverID consumedPDUID) {
    CCL_STATECHECK(eRunMeasurement);

    VAbstractConsumedServicePDU *consumedPDU;
    int32_t result = VAbstractConsumedServicePDU::LookupConsumedPDU(consumedPDUID, consumedPDU);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedPDU->mAbstractConsumedPDU->RequestPDU() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAbstractReleasePDU(cclPDUReceiverID consumedPDUID) {
    CCL_STATECHECK(eRunMeasurement);

    VAbstractConsumedServicePDU *consumedPDU;
    int32_t result = VAbstractConsumedServicePDU::LookupConsumedPDU(consumedPDUID, consumedPDU);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedPDU->mAbstractConsumedPDU->ReleasePDU() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAbstractIsPDURequested(cclPDUReceiverID consumedPDUID, bool *isRequested) {
    CCL_STATECHECK(eRunMeasurement);

    if (isRequested == nullptr)
        return CCL_PARAMETERINVALID;

    VAbstractConsumedServicePDU *consumedPDU;
    int32_t result = VAbstractConsumedServicePDU::LookupConsumedPDU(consumedPDUID, consumedPDU);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedPDU->mAbstractConsumedPDU->IsPDURequested(isRequested) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAbstractSubscribePDU(cclPDUReceiverID consumedPDUID) {
    CCL_STATECHECK(eRunMeasurement);

    VAbstractConsumedServicePDU *consumedPDU;
    int32_t result = VAbstractConsumedServicePDU::LookupConsumedPDU(consumedPDUID, consumedPDU);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedPDU->mAbstractConsumedPDU->SubscribePDU() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAbstractUnsubscribePDU(cclPDUReceiverID consumedPDUID) {
    CCL_STATECHECK(eRunMeasurement);

    VAbstractConsumedServicePDU *consumedPDU;
    int32_t result = VAbstractConsumedServicePDU::LookupConsumedPDU(consumedPDUID, consumedPDU);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedPDU->mAbstractConsumedPDU->UnsubscribePDU() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAbstractSetPDUSubscribedHandler(cclPDUProviderID pduProviderID, cclAbstractPDUSubscriptionHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VAbstractServicePDUProvider *pduProvider;
    int32_t result = VAbstractServicePDUProvider::LookupPDUProvider(pduProviderID, pduProvider);
    if (result != CCL_SUCCESS)
        return result;

    if (pduProvider->SetPDUSubscribedHandler(handler) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t
cclAbstractSetPDUUnsubscribedHandler(cclPDUProviderID pduProviderID, cclAbstractPDUSubscriptionHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VAbstractServicePDUProvider *pduProvider;
    int32_t result = VAbstractServicePDUProvider::LookupPDUProvider(pduProviderID, pduProvider);
    if (result != CCL_SUCCESS)
        return result;

    if (pduProvider->SetPDUUnsubscribedHandler(handler) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

cclConsumedFieldID cclAbstractConsumedFieldGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VAbstractConsumedField::CreateConsumedField(path);
}

cclFieldProviderID cclAbstractFieldProviderGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VAbstractFieldProvider::CreateFieldProvider(path);
}

int32_t cclAbstractRequestField(cclConsumedFieldID consumedFieldID) {
    CCL_STATECHECK(eRunMeasurement);

    VAbstractConsumedField *consumedField;
    int32_t result = VAbstractConsumedField::LookupConsumedField(consumedFieldID, consumedField);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedField->mAbstractConsumedField->RequestField() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAbstractReleaseField(cclConsumedFieldID consumedFieldID) {
    CCL_STATECHECK(eRunMeasurement);

    VAbstractConsumedField *consumedField;
    int32_t result = VAbstractConsumedField::LookupConsumedField(consumedFieldID, consumedField);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedField->mAbstractConsumedField->ReleaseField() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAbstractIsFieldRequested(cclConsumedFieldID consumedFieldID, bool *isRequested) {
    CCL_STATECHECK(eRunMeasurement);

    if (isRequested == nullptr)
        return CCL_PARAMETERINVALID;

    VAbstractConsumedField *consumedField;
    int32_t result = VAbstractConsumedField::LookupConsumedField(consumedFieldID, consumedField);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedField->mAbstractConsumedField->IsFieldRequested(isRequested) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAbstractSubscribeField(cclConsumedFieldID consumedFieldID) {
    CCL_STATECHECK(eRunMeasurement);

    VAbstractConsumedField *consumedField;
    int32_t result = VAbstractConsumedField::LookupConsumedField(consumedFieldID, consumedField);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedField->mAbstractConsumedField->SubscribeField() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclAbstractUnsubscribeField(cclConsumedFieldID consumedFieldID) {
    CCL_STATECHECK(eRunMeasurement);

    VAbstractConsumedField *consumedField;
    int32_t result = VAbstractConsumedField::LookupConsumedField(consumedFieldID, consumedField);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedField->mAbstractConsumedField->UnsubscribeField() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t
cclAbstractSetFieldSubscribedHandler(cclFieldProviderID providerID, cclAbstractFieldSubscriptionHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VAbstractFieldProvider *fieldProvider;
    int32_t result = VAbstractFieldProvider::LookupFieldProvider(providerID, fieldProvider);
    if (result != CCL_SUCCESS)
        return result;

    if (fieldProvider->SetFieldSubscribedHandler(handler) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t
cclAbstractSetFieldUnsubscribedHandler(cclFieldProviderID providerID, cclAbstractFieldSubscriptionHandler handler) {
    CCL_STATECHECK(eRunMeasurement);

    VAbstractFieldProvider *fieldProvider;
    int32_t result = VAbstractFieldProvider::LookupFieldProvider(providerID, fieldProvider);
    if (result != CCL_SUCCESS)
        return result;

    if (fieldProvider->SetFieldUnsubscribedHandler(handler) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

#pragma endregion

#pragma region SOME/IP Binding

cclAddressID cclSomeIPCreateAddress(const char *udpAddress, int32_t udpPort, const char *tcpAddress, int32_t tcpPort) {
    CCL_STATECHECK(eRunMeasurement);

    VIAFbViaService *fbAPI = GetFunctionBusAPI();
    VIAFbSomeIPBinding *someIPBinding;
    if (fbAPI->GetSomeIPBinding(&someIPBinding) != kVIA_OK)
        return CCL_INTERNALERROR;

    NDetail::VIAObjectGuard <VIAFbAddressHandle> address;
    if (someIPBinding->CreateAddress(udpAddress, udpPort, tcpAddress, tcpPort, &(address.mVIAObject)) != kVIA_OK)
        return CCL_CANNOTCREATEADDRESS;

    return VAddress::CreateAddress(address.Release(), true);
}

int32_t cclSomeIPGetBindingID(cclAddressID addressID, int32_t *bindingID) {
    CCL_STATECHECK(eRunMeasurement);

    VIAFbViaService *fbAPI = GetFunctionBusAPI();
    VIAFbSomeIPBinding *someIPBinding;
    if (fbAPI->GetSomeIPBinding(&someIPBinding) != kVIA_OK)
        return CCL_INTERNALERROR;

    VAddress *address;
    int32_t result = VAddress::LookupAddress(addressID, address);
    if (result != CCL_SUCCESS)
        return result;

    VIAFbBindingId fbBindingID;
    if (someIPBinding->GetBindingId(address->mAddress, &fbBindingID) != kVIA_OK)
        return CCL_INVALIDBINDING;

    *bindingID = static_cast<int32_t>(fbBindingID);
    return CCL_SUCCESS;
}

int32_t cclSomeIPGetDisplayName(cclAddressID addressID, char *buffer, int32_t bufferSize) {
    CCL_STATECHECK(eRunMeasurement);

    VIAFbViaService *fbAPI = GetFunctionBusAPI();
    VIAFbSomeIPBinding *someIPBinding;
    if (fbAPI->GetSomeIPBinding(&someIPBinding) != kVIA_OK)
        return CCL_INTERNALERROR;

    VAddress *address;
    int32_t result = VAddress::LookupAddress(addressID, address);
    if (result != CCL_SUCCESS)
        return result;

    result = someIPBinding->GetDisplayName(address->mAddress, buffer, bufferSize);
    if (result == kVIA_BufferToSmall)
        return CCL_BUFFERTOSMALL;

    return (result == kVIA_OK) ? CCL_SUCCESS : CCL_INTERNALERROR;
}

cclSomeIPConsumedEventGroupID cclSomeIPConsumedEventGroupGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VConsumedEventGroup::CreateConsumedEventGroup(path);
}

cclSomeIPProvidedEventGroupID cclSomeIPProvidedEventGroupGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VProvidedEventGroup::CreateProvidedEventGroup(path);
}

cclSomeIPEventGroupProviderID cclSomeIPEventGroupProviderGetID(const char *path) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    return VEventGroupProvider::CreateEventGroupProvider(path);
}

int32_t cclSomeIPRequestEventGroup(cclSomeIPConsumedEventGroupID consumedEventGroupID) {
    CCL_STATECHECK(eRunMeasurement);

    VConsumedEventGroup *consumedEventGroup;
    int32_t result = VConsumedEventGroup::LookupConsumedEventGroup(consumedEventGroupID, consumedEventGroup);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedEventGroup->mPort->RequestEventGroup() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclSomeIPReleaseEventGroup(cclSomeIPConsumedEventGroupID consumedEventGroupID) {
    CCL_STATECHECK(eRunMeasurement);

    VConsumedEventGroup *consumedEventGroup;
    int32_t result = VConsumedEventGroup::LookupConsumedEventGroup(consumedEventGroupID, consumedEventGroup);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedEventGroup->mPort->ReleaseEventGroup() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclSomeIPIsEventGroupRequested(cclSomeIPConsumedEventGroupID consumedEventGroupID, bool *isRequested) {
    CCL_STATECHECK(eRunMeasurement);

    if (isRequested == nullptr)
        return CCL_PARAMETERINVALID;

    VConsumedEventGroup *consumedEventGroup;
    int32_t result = VConsumedEventGroup::LookupConsumedEventGroup(consumedEventGroupID, consumedEventGroup);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedEventGroup->mPort->IsEventGroupRequested(isRequested) != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t
cclSomeIPConsumedEventGroupGetEvents(cclSomeIPConsumedEventGroupID consumedEventGroupID, cclConsumedEventID *buffer,
                                     int32_t *bufferSize) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VConsumedEventGroup *consumedEventGroup;
    int32_t result = VConsumedEventGroup::LookupConsumedEventGroup(consumedEventGroupID, consumedEventGroup);
    if (result != CCL_SUCCESS)
        return result;

    return consumedEventGroup->GetEvents(buffer, bufferSize);
}

int32_t
cclSomeIPProvidedEventGroupGetEvents(cclSomeIPProvidedEventGroupID providedEventGroupID, cclProvidedEventID *buffer,
                                     int32_t *bufferSize) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VProvidedEventGroup *providedEventGroup;
    int32_t result = VProvidedEventGroup::LookupProvidedEventGroup(providedEventGroupID, providedEventGroup);
    if (result != CCL_SUCCESS)
        return result;

    return providedEventGroup->GetEvents(buffer, bufferSize);
}

int32_t
cclSomeIPEventGroupProviderGetEvents(cclSomeIPEventGroupProviderID eventGroupProviderID, cclEventProviderID *buffer,
                                     int32_t *bufferSize) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VEventGroupProvider *eventGroupProvider;
    int32_t result = VEventGroupProvider::LookupEventGroupProvider(eventGroupProviderID, eventGroupProvider);
    if (result != CCL_SUCCESS)
        return result;

    return eventGroupProvider->GetEvents(buffer, bufferSize);
}

int32_t
cclSomeIPConsumedEventGroupGetPDUs(cclSomeIPConsumedEventGroupID consumedEventGroupID, cclConsumedEventID *buffer,
                                   int32_t *bufferSize) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VConsumedEventGroup *consumedEventGroup;
    int32_t result = VConsumedEventGroup::LookupConsumedEventGroup(consumedEventGroupID, consumedEventGroup);
    if (result != CCL_SUCCESS)
        return result;

    return consumedEventGroup->GetPDUs(buffer, bufferSize);
}

int32_t
cclSomeIPProvidedEventGroupGetPDUs(cclSomeIPProvidedEventGroupID providedEventGroupID, cclProvidedEventID *buffer,
                                   int32_t *bufferSize) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VProvidedEventGroup *providedEventGroup;
    int32_t result = VProvidedEventGroup::LookupProvidedEventGroup(providedEventGroupID, providedEventGroup);
    if (result != CCL_SUCCESS)
        return result;

    return providedEventGroup->GetPDUs(buffer, bufferSize);
}

int32_t
cclSomeIPEventGroupProviderGetPDUs(cclSomeIPEventGroupProviderID eventGroupProviderID, cclEventProviderID *buffer,
                                   int32_t *bufferSize) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VEventGroupProvider *eventGroupProvider;
    int32_t result = VEventGroupProvider::LookupEventGroupProvider(eventGroupProviderID, eventGroupProvider);
    if (result != CCL_SUCCESS)
        return result;

    return eventGroupProvider->GetPDUs(buffer, bufferSize);
}

int32_t
cclSomeIPConsumedEventGroupGetFields(cclSomeIPConsumedEventGroupID consumedEventGroupID, cclConsumedFieldID *buffer,
                                     int32_t *bufferSize) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VConsumedEventGroup *consumedEventGroup;
    int32_t result = VConsumedEventGroup::LookupConsumedEventGroup(consumedEventGroupID, consumedEventGroup);
    if (result != CCL_SUCCESS)
        return result;

    return consumedEventGroup->GetFields(buffer, bufferSize);
}

int32_t
cclSomeIPProvidedEventGroupGetFields(cclSomeIPProvidedEventGroupID providedEventGroupID, cclProvidedFieldID *buffer,
                                     int32_t *bufferSize) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VProvidedEventGroup *providedEventGroup;
    int32_t result = VProvidedEventGroup::LookupProvidedEventGroup(providedEventGroupID, providedEventGroup);
    if (result != CCL_SUCCESS)
        return result;

    return providedEventGroup->GetFields(buffer, bufferSize);
}

int32_t
cclSomeIPEventGroupProviderGetFields(cclSomeIPEventGroupProviderID eventGroupProviderID, cclFieldProviderID *buffer,
                                     int32_t *bufferSize) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VEventGroupProvider *eventGroupProvider;
    int32_t result = VEventGroupProvider::LookupEventGroupProvider(eventGroupProviderID, eventGroupProvider);
    if (result != CCL_SUCCESS)
        return result;

    return eventGroupProvider->GetFields(buffer, bufferSize);
}

int32_t cclSomeIPEventGroupGetNrOfSubscribedConsumers(cclSomeIPEventGroupProviderID eventGroupProviderID,
                                                      int32_t *nrOfConsumers) {
    CCL_STATECHECK(eRunMeasurement);

    VEventGroupProvider *eventGroupProvider;
    int32_t result = VEventGroupProvider::LookupEventGroupProvider(eventGroupProviderID, eventGroupProvider);
    if (result != CCL_SUCCESS)
        return result;

    return eventGroupProvider->GetNrOfSubscribedConsumers(nrOfConsumers);
}

int32_t cclSomeIPEventGroupGetSubscribedConsumers(cclSomeIPEventGroupProviderID eventGroupProviderID,
                                                  cclConsumerID *consumerBuffer, int32_t *bufferSize) {
    CCL_STATECHECK(eRunMeasurement);

    VEventGroupProvider *eventGroupProvider;
    int32_t result = VEventGroupProvider::LookupEventGroupProvider(eventGroupProviderID, eventGroupProvider);
    if (result != CCL_SUCCESS)
        return result;

    return eventGroupProvider->GetSubscribedConsumers(consumerBuffer, bufferSize);
}

int32_t cclSomeIPSubscribeEventGroup(cclSomeIPConsumedEventGroupID consumedEventGroupID) {
    CCL_STATECHECK(eRunMeasurement);

    VConsumedEventGroup *consumedEventGroup;
    int32_t result = VConsumedEventGroup::LookupConsumedEventGroup(consumedEventGroupID, consumedEventGroup);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedEventGroup->mPort->SubscribeEventGroup() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclSomeIPUnsubscribeEventGroup(cclSomeIPConsumedEventGroupID consumedEventGroupID) {
    CCL_STATECHECK(eRunMeasurement);

    VConsumedEventGroup *consumedEventGroup;
    int32_t result = VConsumedEventGroup::LookupConsumedEventGroup(consumedEventGroupID, consumedEventGroup);
    if (result != CCL_SUCCESS)
        return result;

    if (consumedEventGroup->mPort->UnsubscribeEventGroup() != kVIA_OK)
        return CCL_INTERNALERROR;

    return CCL_SUCCESS;
}

int32_t cclSomeIPSetEventGroupSubscribedHandler(cclSomeIPEventGroupProviderID eventGroupProviderID,
                                                cclSomeIPEventGroupSubscriptionHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VEventGroupProvider *eventGroupProvider;
    int32_t result = VEventGroupProvider::LookupEventGroupProvider(eventGroupProviderID, eventGroupProvider);
    if (result != CCL_SUCCESS)
        return result;

    return eventGroupProvider->SetEventGroupSubscribedHandler(handler);
}

int32_t cclSomeIPSetEventGroupUnsubscribedHandler(cclSomeIPEventGroupProviderID eventGroupProviderID,
                                                  cclSomeIPEventGroupSubscriptionHandler handler) {
    CCL_STATECHECK(eInitMeasurement | eRunMeasurement | eStopMeasurement);

    VEventGroupProvider *eventGroupProvider;
    int32_t result = VEventGroupProvider::LookupEventGroupProvider(eventGroupProviderID, eventGroupProvider);
    if (result != CCL_SUCCESS)
        return result;

    return eventGroupProvider->SetEventGroupUnsubscribedHandler(handler);
}

#pragma endregion

#pragma region ADAS

VIAADASService *GetADASService() {
    if (gADASService == nullptr) {
        gVIAService->GetADASService(&gADASService);
    }
    return gADASService;
}

int32_t cclSetGpbOSI3Data(uint8_t *data, int32_t dataSize) {
    return cclSetGpbOSI3Data2(data, dataSize, 0);
}

int32_t cclSetGpbOSI3Data2(uint8_t *data, int32_t dataSize, int32_t dataType) {
    CCL_STATECHECK(eRunMeasurement);

    VIAADASService *lADASService = nullptr;
    NADAS::IADASDataAccess *lADASAccess = nullptr;
    lADASService = GetADASService();
    lADASService->GetADASDataAccess(&lADASAccess);
    if (lADASAccess != nullptr) {
        return (lADASAccess->SetGpbOSI3Data2(data, dataSize, dataType) ? CCL_SUCCESS : CCL_INVALIDDATA);
    }
    return CCL_INTERNALERROR;
}

#pragma endregion

#pragma region Ethernet Packets

// ============================================================================
// Ethernet
// ============================================================================


int32_t cclEthernetOutputPacket(int32_t channel, uint32_t packetSize, const uint8_t data[]) {
    CCL_STATECHECK(eRunMeasurement)

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    VIAEthernetBus *ethernetBus = gEthernetBusContext[channel].mBus;
    if (ethernetBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    bool isSwitchedNetwork = false;
    if (ethernetBus->IsSwitchedNetwork(isSwitchedNetwork) != kVIA_OK) {
        return CCL_NOTSUPPORTED;
    }
    if (isSwitchedNetwork) {
        return CCL_NOTSUPPORTED;
    }

    VIAResult rc = ethernetBus->OutputEthernetPacket(channel, packetSize, data);
    if (rc != kVIA_OK) {
        return CCL_INTERNALERROR;
    }

    return CCL_SUCCESS;
}


int32_t cclEthernetSetPacketHandler(int32_t channel, void(*function)(struct cclEthernetPacket *packet)) {
    CCL_STATECHECK(eInitMeasurement)

    if (channel < 1 || channel > cMaxChannel) {
        return CCL_INVALIDCHANNEL;
    }

    if (gEthernetBusContext[channel].mBus == nullptr) {
        return CCL_INVALIDCHANNEL;
    }

    if (function == nullptr) {
        return CCL_INVALIDFUNCTIONPOINTER;
    }

    bool isSwitchedNetwork = false;
    if (gEthernetBusContext[channel].mBus->IsSwitchedNetwork(isSwitchedNetwork) != kVIA_OK) {
        return CCL_NOTSUPPORTED;
    }
    if (isSwitchedNetwork) {
        return CCL_NOTSUPPORTED;
    }

    VEthernetPacketRequest *request = new VEthernetPacketRequest;
    request->mCallbackFunction = function;
    request->mContext = &gEthernetBusContext[channel];
    request->mHandle = nullptr;

    // The library only supports channel based Ethernet setup, thus CreateThisNodePacketRequest is not used.
    VIAResult rc = gEthernetBusContext[channel].mBus->CreateAllPacketRequest(&(request->mHandle), request, channel);
    if (rc != kVIA_OK) {
        delete request;
        if (rc == kVIA_ServiceNotRunning) {
            return CCL_WRONGSTATE;
        } else {
            return CCL_INTERNALERROR;
        }
    }

    gModule->mEthernetPacketRequests.push_back(request);
    return CCL_SUCCESS;
}

int32_t cclEthernetGetChannelNumber(const char *networkName, int32_t *channel) {
    if (!gHasBusNames) {
        return CCL_NOTIMPLEMENTED;
    }

    if (networkName == nullptr || strlen(networkName) == 0) {
        return CCL_INVALIDNAME;
    }

    for (int32_t i = 1; i <= cMaxChannel; i++) {
        if (gEthernetBusContext[i].mBusName == networkName) {
            if (channel != nullptr) {
                *channel = i;
            }
            return CCL_SUCCESS;
        }
    }

    return CCL_NETWORKNOTDEFINED; // There is no Ethernet network with the given name.
}

#pragma endregion

