/*-----------------------------------------------------------------------------
Module: VIA (not observed)
Interfaces: This is a public interface header.
-------------------------------------------------------------------------------
Public interface for access to the FunctionBus
-------------------------------------------------------------------------------
Copyright (c) Vector Informatik GmbH. All rights reserved.
-----------------------------------------------------------------------------*/

#ifndef VIA_FUNCTIONBUS_H
#define VIA_FUNCTIONBUS_H

/*-----------------------------------------------------------------------------
Version of the interfaces, which are declared in this header

2016-11-16   1.0  Arr     Creation
2017-03-24   1.1  Who     Extended VE access, service discovery features
2017-04-04   1.2  Who     Implementation of PDU access and service discovery
2017-04-11   1.3  Who     Binding Block specific API (events and event groups)
2017-04-20   1.4  Who     Access client/server FEP through service ports
2017-05-08   1.5  Who     Enhancements to call context and VE access
2017-06-20   1.6  Who     Fixed VS 2010 compatibility issue
2017-06-28   1.7  Rue     Added ResetStatus to VIAFBStatus
2017-08-03   1.8  Who     Observe removal of dynamic FEPs and ports
2017-10-10   1.9  Who     Field support
2018-11-05   1.10 Rue     Unbounded arrays, generic PDU signals and measurement points
2019-04-16   1.11 Arr     Support for Distributed Objects
2020-03-26   1.12 Rue     DO interface extensions
2020-05-25   1.13 Rue     DO binding block extensions
2020-08-17   1.14 Hph     Added method GetSubMemberName
2020-08-21   1.15 Rue     Added IsBoolean to VIAFbType
-----------------------------------------------------------------------------*/

#define VIAFunctionBusServiceMajorVersion 1
#define VIAFunctionBusServiceMinorVersion 17

// ============================================================================
// Dependencies
// ============================================================================

#ifndef VIA_H
#include "VIA.h"
#endif

#include <cstddef>

// Forward declarations
class VIAFbViaService;

class VIAFbValuePort;

class VIAFbSignalSenderPort;

class VIAFbSignalReceiverPort;

class VIAFbPDUSenderPort;

class VIAFbPDUReceiverPort;

class VIAFbFunctionClientPort;

class VIAFbFunctionServerPort;

class VIAFbCallContext;

class VIAFbServiceConsumer;

class VIAFbServiceProvider;

class VIAFbConsumedService;

class VIAFbProvidedService;

class VIAFbEventProvider;

class VIAFbConsumedEvent;

class VIAFbProvidedEvent;

class VIAFbServicePDUProvider;

class VIAFbConsumedServicePDU;

class VIAFbProvidedServicePDU;

class VIAFbFieldProvider;

class VIAFbConsumedField;

class VIAFbProvidedField;

class VIAFbDO;

class VIAFbDOInterface;

class VIAFbDOMember;

class VIAFbDOMemberInterface;

class VIAFbDOMemberValueObserver;

class VIAFbDOMethod;

class VIAFbDOMethodProvider;

class VIAFbDOMethodConsumer;

class VIAFbDOConsumedMethod;

class VIAFbDOProvidedMethod;

class VIAFbDOInternalMethod;

class VIAFbDOData;

class VIAFbDODataProvider;

class VIAFbDODataConsumer;

class VIAFbDOConsumedData;

class VIAFbDOProvidedData;

class VIAFbDOInternalData;

class VIAFbDOValue;

class VIAFbDOValueProvider;

class VIAFbDOValueConsumer;

class VIAFbDOEvent;

class VIAFbDOEventProvider;

class VIAFbDOEventConsumer;

class VIAFbDOConsumedEvent;

class VIAFbDOProvidedEvent;

class VIAFbDOInternalEvent;

class VIAFbDOField;

class VIAFbDOFieldProvider;

class VIAFbDOFieldConsumer;

class VIAFbDOConsumedField;

class VIAFbDOProvidedField;

class VIAFbDOInternalField;

class VIAFbDONetworkAttached;

class VIAFbDOSubscriber;

class VIAFbDOAnnouncer;

class VIAFbDOBinding;

class VIAFbDONetwork;

class VIAFbDODiscovery;

class VIAFbDOBlueprint;

class VIAFbDOContainer;

class VIAFbServiceCO;

class VIAFbAbstractBinding;

class VIAFbAbstractEventProvider;

class VIAFbAbstractConsumedEvent;

class VIAFbAbstractServicePDUProvider;

class VIAFbAbstractConsumedServicePDU;

class VIAFbAbstractFieldProvider;

class VIAFbAbstractConsumedField;

class VIAFbSomeIPBinding;

class VIAFbSomeIPEventGroupProvider;

class VIAFbSomeIPConsumedEventGroup;

class VIAFbSomeIPProvidedEventGroup;

class VIAFbValue;

class VIAFbStatus;

class VIAFbType;

// ============================================================================
// Type definitions and constants
// ============================================================================

// FunctionBus types
enum VIAFbTypeTag {
    eSignedInteger,    // Encoding: 2C
    eUnsignedInteger,  // Encoding: None
    eFloat,            // Encoding: IEEE754
    eString,           // Encoding: To be determined by GetEncoding
    eArray,
    eStruct,
    eUnion,
    eData,
    eVoid
};

// The representational type used to access a value
enum VIAFbTypeLevel {
    ePhys,  // Physical representation, restricted to Integer and Float
    eImpl,  // Implementation representation
    eRaw,   // Bit-serialized representation
};

enum VIAFbValueClass {
    ePortValue,         // The value entity directly related with the addressed PDU, Signal or Event port. Also used for DO values.
    eServiceState,      // Service state of Service provider side FEP
    eConnectionState,   // Connection state of Service AP
    eSubscriptionState, // Subscription state of PDU/Event AP or MP for COs; or of Data / Event for DOs
    eLatestCall,        // Latest call input parameters of function AP or MP or DO member
    eLatestReturn,      // Latest call return parameters of function AP or MP or DO member
    eParamDefaults,     // VSIM configuration of function provider side AP or DO member
    eAnnouncementState  // Announcement state of Data / Event (DOs only)
};

// Mode with which notifications about value changes are sent
enum VIAFbUpdateMode {
    eNotifyOnUpdate,  // Notify whenever value is set (on update)
    eNotifyOnChange,  // Notify whenever value is set and the new value is different from the old value (on change)
};

// States a FunctionBus service call can be in
enum VIAFbFunctionCallState {
    eUndefinedState,
    eCalling,
    eCalled,
    eReturning,
    eReturned,
    eFinalizing,
};

// Whether a call context is being processed on client or server side
enum VIAFbFunctionCallSide {
    eClient,
    eServer
};

// Whether a value entity (element) was last measured or is still offline
enum VIAFbValueState {
    eOffline,
    eMeasured
};

// Dynamically added FEP state
enum VIAFbFEPState {
    eSimulated,
    eReal
};

// Service state at service provider
enum VIAFbServiceState {
    eUnavailable,
    eInitializing,
    eAvailable,
    eUnknown
};

// Connection state at consumer side port
enum VIAFbConsumerConnectionState {
    eProviderUnavailable,
    eProviderConnectable,
    eProviderAvailable
};

// Connection state at provider side port
enum VIAFbProviderConnectionState {
    eConsumerUnavailable,
    eConsumerConnectable,
    eConsumerConnected
};

// PDU, event and event group subscription state
enum VIAFbSubscriptionState {
    eSubscribed,
    eSubscribable,
    eUnsubscribable
};

// Whether a DO instance's member can be provided or consumed
enum VIAFbDOMemberDirection {
    eFbDOMemberDirection_Provided,
    eFbDOMemberDirection_Consumed,
    eFbDOMemberDirection_Internal
};

// Type of a DO member (not the data type, the member type)
enum VIAFbDOMemberType {
    eFbDOMemberType_DataMember,
    eFbDOMemberType_Method,
    eFbDOMemberType_Event,
    eFbDOMemberType_Field
};

// To retrieve a specific interface from a member
enum VIAFbDOMemberInterfaceType {
    // for all member types
    eFbDOMemberIF_Generic,
    // methods ...
    eFbDOMemberIF_Method,
    eFbDOMemberIF_MethodProvider,
    eFbDOMemberIF_MethodConsumer,
    eFbDOMemberIF_ProvidedMethod,
    eFbDOMemberIF_ConsumedMethod,
    eFbDOMemberIF_InternalMethod,
    // data or events or fields ...
    eFbDOMemberIF_Value,
    eFbDOMemberIF_ValueProvider,
    eFbDOMemberIF_ValueConsumer,
    // data ...
    eFbDOMemberIF_Data,
    eFbDOMemberIF_DataProvider,
    eFbDOMemberIF_DataConsumer,
    eFbDOMemberIF_ConsumedData,
    eFbDOMemberIF_ProvidedData,
    eFbDOMemberIF_InternalData,
    // events ...
    eFbDOMemberIF_Event,
    eFbDOMemberIF_EventProvider,
    eFbDOMemberIF_EventConsumer,
    eFbDOMemberIF_ConsumedEvent,
    eFbDOMemberIF_ProvidedEvent,
    eFbDOMemberIF_InternalEvent,
    // fields ...
    eFbDOMemberIF_Field,
    eFbDOMemberIF_FieldProvider,
    eFbDOMemberIF_FieldConsumer,
    eFbDOMemberIF_ConsumedField,
    eFbDOMemberIF_ProvidedField,
    eFbDOMemberIF_InternalField,
    // network management
    eFbDOMemberIF_NetworkAttached,
    // communication pattern specific
    eFbDOMemberIF_Subscriber,
    eFbDOMemberIF_Announcer,
};

// Address handle internally maps to an NFunctionBus::AddressHandle
struct VIAFbAddressHandle {
    VIASTDDECL Release() = 0;
};

// Callback handle internally maps to an ICallback object
typedef void *VIAFbCallbackHandle;

// Type handle internally maps to a DataTypeMemberHandle from SystemVariables.h:
typedef int32 VIAFbTypeMemberHandle;
const VIAFbTypeMemberHandle kVIAFbTypeMemberHandleWhole = VIAFbTypeMemberHandle(-1);
const VIAFbTypeMemberHandle kVIAFbTypeMemberHandleInvalid = VIAFbTypeMemberHandle(-2);

// Binding specific identifier of service consumer and provider
typedef int32 VIAFbBindingId;
const VIAFbBindingId kVIAFbBindingIdInvalid = VIAFbBindingId(-1);

// Time constants
typedef VIATime VIATimeDiff;
const VIATimeDiff kVIAFbTimeDeltaNow(0);
const VIATimeDiff kVIAFbTimeDeltaNever(-1);
const VIATime kVIAFbTimeUndefined(-1);

// ============================================================================
// Observer classes
// ============================================================================

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)

// Callback handler to notify observers when dynamic function bus ports or FEPs are removed.
class VIAFbPortObserver {
public:
    // Indicates that a dynamic port or FEP is about to be removed
    // [OUT] Return value is ignored
    VIASTDDECL OnPortRemoval(
            uint64 portID) = 0;                      // [IN] direction of port being removed
};

#endif

// Callback handler to notify observers whenever a FunctionBus signal port receives a value.
// Note: this is identical to observing value updates of the signal receiver port VE.
class VIAFbSignalReceiverObserver {
public:
    // Indicates that the signal value at the receiver port has changed
    // [OUT] Return value is ignored
    VIASTDDECL OnSignalReceived(
            VIATime inTime,                                      // [IN] Absolute time the signal value was updated
            const VIAFbSignalReceiverPort *inReceiverPort) = 0;  // [IN] Receiving signal port
};

// Callback handler to notify observers whenever a FunctionBus PDU port receives a value.
// Note: this is identical to observing value updates of the signal receiver port VE.
class VIAFbPDUReceiverObserver {
public:
    // Indicates that the PDU value at the receiver port has changed
    // [OUT] Return value is ignored
    VIASTDDECL OnPDUReceived(
            VIATime inTime,                                   // [IN] Absolute time the PDU value was updated
            const VIAFbPDUReceiverPort *inReceiverPort) = 0;  // [IN] Receiving PDU port
};

// Callback handler to notify observers about any service discovery events.
class VIAFbServiceDiscoveryObserver {
public:
    // Indicates that a new service discovery request was sent by some service consumer.
    // [OUT] Return value is ignored
    VIASTDDECL OnServiceDiscovery(
            VIAFbServiceCO *inService,                        // [IN] The service hosting the requesting consumer
            VIAFbServiceConsumer *inConsumer) = 0;            // [IN] The service consumer which initiated service discovery

    // Indicates that a new service consumer was discovered
    // [OUT] Return value is ignored
    VIASTDDECL OnConsumerDiscovered(
            VIAFbServiceCO *inService,                        // [IN] The service hosting the discovered consumer
            VIAFbAddressHandle *inAddress) = 0;               // [IN] The (binding specific) address of the discovered consumer

    // Indicates that a new service provider was discovered
    // [OUT] Return value is ignored
    VIASTDDECL OnProviderDiscovered(
            VIAFbServiceCO *inService,                        // [IN] The service hosting the discovered provider
            VIAFbAddressHandle *inAddress) = 0;               // [IN] The (binding specific) address of the discovered provider
};

// Callback handler to notify observers about discovered consumers and connections for a specific service provider
class VIAFbServiceProviderObserver {
public:
    // Indicates that a new service consumer was discovered
    // [OUT] Return value is ignored
    VIASTDDECL OnConsumerDiscovered(
            VIAFbServiceProvider *inProvider,                 // [IN] The service provider detecting the consumer
            VIAFbAddressHandle *inAddress) = 0;               // [IN] The (binding specific) address of the discovered consumer

    // Indicates that a consumer requests a connection to the provider
    // [OUT] Return value is ignored
    VIASTDDECL OnConnectionRequested(
            VIAFbServiceProvider *inProvider,                 // [IN] The service provider receiving the connection request
            VIAFbServiceConsumer *inConsumer) = 0;            // [IN] The service consumer requesting the connection
};

// Callback handler to notify observers about discovered providers for a specific service consumer
class VIAFbServiceConsumerObserver {
public:
    // Indicates that a new service provider was discovered
    // [OUT] Return value is ignored
    VIASTDDECL OnProviderDiscovered(
            VIAFbServiceConsumer *inConsumer,                 // [IN] The service consumer detecting the provider
            VIAFbAddressHandle *inAddress) = 0;               // [IN] The (binding specific) address of the discovered provider
};

// Callback handler to notify about service connection success or failure
class VIAFbConsumedServiceConnectionHandler {
public:
    // Indicates that the service connection was successfully established
    // [OUT] Return value is ignored
    VIASTDDECL OnConnectionEstablished(
            VIAFbConsumedService *inPort) = 0;                // [IN] the consumer side service port which is now connected

    // Indicates that the service connection could not be established
    // [OUT] Return value is ignored
    VIASTDDECL OnConnectionFailed(
            VIAFbConsumedService *inPort,                     // [IN] the consumer side service port which failed to connect
            const char *inError) = 0;                         // [IN] An error message indicating the cause of connection failure
};

// Callback handler to notify about service connection success or failure
class VIAFbProvidedServiceConnectionHandler {
public:
    // Indicates that the service connection was successfully established
    // [OUT] Return value is ignored
    VIASTDDECL OnConnectionEstablished(
            VIAFbProvidedService *inPort) = 0;                // [IN] the provider side service port which is now connected

    // Indicates that the service connection could not be established
    // [OUT] Return value is ignored
    VIASTDDECL OnConnectionFailed(
            VIAFbProvidedService *inPort,                     // [IN] the provider side service port which failed to connect
            const char *inError) = 0;                         // [IN] An error message indicating the cause of connection failure
};

// Callback handler to notify client-side observers when the called server-side FunctionBus service port receives the call or either an error or the result arrives.
class VIAFbFunctionClientObserver {
public:
    // Indicates that the invocation initiated at the client port changed its state
    // Note: The obtained inCallContext must be released explicitly via inCallContext->Release.
    // [OUT] Return value is ignored
    VIASTDDECL OnCallStateChanged(
            VIATime inTime,                                    // [IN] Absolute time this state change was observed
            VIAFbCallContext *inCallContext,                   // [IN] Call context of the affected call
            VIAFbFunctionCallState inCallState,                // [IN] State reached
            const VIAFbFunctionClientPort *inClientPort) = 0;  // [IN] Client function port
};

// Callback handler to notify server-side observers whenever a FunctionBus service port receives a call from a client to be handled.
class VIAFbFunctionServerObserver {
public:
    // Indicates that an invocation request arrived at the server port
    // Note: The obtained inCallContext must be released explicitly via inCallContext->Release.
    // [OUT] Return value is ignored
    VIASTDDECL OnCallStateChanged(
            VIATime inTime,                                    // [IN] Absolute time this state change was observed
            VIAFbCallContext *inCallContext,                   // [IN] Call context of the affected call
            VIAFbFunctionCallState inCallState,                // [IN] State reached
            const VIAFbFunctionServerPort *inServerPort) = 0;  // [IN] Server function port
};

// Callback handler to notify value entity (element) observers when the value was updated or changed
class VIAFbValueObserver {
public:
    // Indicates that the value of the value entity (element) was updated
    // Note: this callback is used for value updates the same as for value changes, which events should fire must be specified upon registration.
    VIASTDDECL OnValueUpdated(
            VIATime inTime,                                    // [IN] Absolute time this value update was observed
            VIAFbStatus *inStatus) = 0;                        // [IN] Updated value entity status object (provides access to the current value)
};

// Callback handler to notify observers about event subscription and desubscription
class VIAFbAbstractEventSubscriptionObserver {
public:
    // Indicates that an event consumer has subscribed at a specific provider.
    // [OUT] Return value is ignored
    VIASTDDECL OnEventSubscribed(
            VIAFbProvidedEvent *inEventPort) = 0;             // [IN] The event provider side port reflecting the subscribed connection

    // Indicates that an event consumer has unsubscribed from a specific provider.
    // [OUT] Return value is ignored
    VIASTDDECL OnEventUnsubscribed(
            VIAFbProvidedEvent *inEventPort) = 0;             // [IN] The event provider side port reflecting the no more subscribed connection
};

// Callback handler to notify observers about PDU subscription and desubscription
class VIAFbAbstractPDUSubscriptionObserver {
public:
    // Indicates that a PDU consumer has subscribed at a specific provider.
    // [OUT] Return value is ignored
    VIASTDDECL OnPDUSubscribed(
            VIAFbProvidedServicePDU *inPDUPort) = 0;          // [IN] The PDU provider side port reflecting the subscribed connection

    // Indicates that a PDU consumer has unsubscribed from a specific provider.
    // [OUT] Return value is ignored
    VIASTDDECL OnPDUUnsubscribed(
            VIAFbProvidedServicePDU *inPDUPort) = 0;          // [IN] The PDU provider side port reflecting the no more subscribed connection
};

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 9)

// Callback handler to notify observers about field subscription and desubscription
class VIAFbAbstractFieldSubscriptionObserver {
public:
    // Indicates that a field consumer has subscribed at a specific provider.
    // [OUT] Return value is ignored
    VIASTDDECL OnFieldSubscribed(
            VIAFbProvidedField *inFieldPort) = 0;             // [IN] The field provider side port reflecting the subscribed connection

    // Indicates that a field consumer has unsubscribed from a specific provider.
    // [OUT] Return value is ignored
    VIASTDDECL OnFieldUnsubscribed(
            VIAFbProvidedField *inFieldPort) = 0;             // [IN] The field provider side port reflecting the no more subscribed connection
};

#endif

// Callback handler to notify observers about event group subscription and desubscription
class VIAFbSomeIPEventGroupSubscriptionObserver {
public:
    // Indicates that an event group consumer has subscribed at a specific provider.
    // [OUT] Return value is ignored
    VIASTDDECL OnEventGroupSubscribed(
            VIAFbSomeIPProvidedEventGroup *inEventGroupPort) = 0;  // [IN] The event group provider side port reflecting the subscribed connection

    // Indicates that an event group consumer has unsubscribed from a specific provider.
    // [OUT] Return value is ignored
    VIASTDDECL OnEventGroupUnsubscribed(
            VIAFbSomeIPProvidedEventGroup *inEventGroupPort) = 0;  // [IN] The event group provider side port reflecting the no more subscribed connection
};

// ============================================================================
// Iterator classes
// ============================================================================

class VIAFbServiceConsumerIterator {
public:
    // Returns whether more consumers can be iterated
    VIABOOLDECL HasMoreConsumers() const = 0;

    // Returns the current consumer and moves the iterator forward
    VIASTDDECL GetNextConsumer(
            VIAFbServiceConsumer **outConsumer) = 0;          // [OUT] The next service consumer instance

    // Go to next consumer without creating
    VIASTDDECL SkipConsumer() = 0;

    // Releases the iterator object
    VIASTDDECL Release() = 0;
};

template<typename T>
class VIAFbEventIterator {
public:
    // Returns whether more events can be iterated
    VIABOOLDECL HasMoreEvents() const = 0;

    // Returns the current event and moves the iterator forward
    VIASTDDECL GetNextEvent(
            T **outEvent) = 0;                                // [OUT] The next service event instance

    // Go to next event without creating
    VIASTDDECL SkipEvent() = 0;

    // Releases the iterator object
    VIASTDDECL Release() = 0;
};

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)

template<typename T>
class VIAFbPDUIterator {
public:
    // Returns whether more PDUs can be iterated
    VIABOOLDECL HasMorePDUs() const = 0;

    // Returns the current PDU and moves the iterator forward
    VIASTDDECL GetNextPDU(
            T **outPDU) = 0;                                // [OUT] The next service PDU instance

    // Go to next PDU without creating
    VIASTDDECL SkipPDU() = 0;

    // Releases the iterator object
    VIASTDDECL Release() = 0;
};

#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 9)

template<typename T>
class VIAFbFieldIterator {
public:
    // Returns whether more fields can be iterated
    VIABOOLDECL HasMoreFields() const = 0;

    // Returns the current field and moves the iterator forward
    VIASTDDECL GetNextField(
            T **outField) = 0;                                // [OUT] The next service field instance

    // Go to next field without creating
    VIASTDDECL SkipField() = 0;

    // Releases the iterator object
    VIASTDDECL Release() = 0;
};

#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 11)

template<typename T>
class VIAFbMemberIterator {
public:
    // Returns whether more members can be iterated
    VIABOOLDECL HasMoreMembers() const = 0;

    // Returns the current member and moves the iterator forward
    VIASTDDECL GetNextMember(
            T **outMember) = 0;                                // [OUT] The next member instance

    // Go to next member without creating
    VIASTDDECL SkipMember() = 0;

    // Releases the iterator object
    VIASTDDECL Release() = 0;
};

template<typename T>
class VIAFbInterfaceIterator {
public:
    // Returns whether more interfaces can be iterated
    VIABOOLDECL HasMoreInterfaces() const = 0;

    // Returns the current interface and moves the iterator forward
    VIASTDDECL GetNextInterface(
            T **outInterface) = 0;                             // [OUT] The next interface instance

    // Go to next interface without creating
    VIASTDDECL SkipInterface() = 0;

    // Releases the iterator object
    VIASTDDECL Release() = 0;
};

#endif

// ============================================================================
// Classes
// ============================================================================

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 11)

// Mode for attribute retrieval
enum VIAFbAttributeRetrievalMode {
    // look only for locally defined attribute values
    kVIAFbAttributeRetrieval_Local = 0,
    // return also inherited attribute values
    kVIAFbAttributeRetrieval_Inherited,
    // as inherited, but also replace placeholders with attribute values
    kVIAFbAttributeRetrieval_Replaced
};

// Result for attribute set operation
enum VIAFbAttributeSetResult {
    // no error
    kVIAFbAttributeSet_OK = 0,
    // attribute is not allowed for the object (type)
    kVIAFbAttributeSet_AttributeNotAllowedForObject,
    // attribute value has the wrong data type regarding the attribute definition
    kVIAFbAttributeSet_ValueHasWrongDataType,
    // there is an invalid placeholder in the value
    kVIAFbAttributeSet_InvalidPlaceholderSyntax,
    // the attribute definition was not found
    kVIAFbAttributeSet_AttributeNotFound,
    // the attribute value can't be changed during measurement
    kVIAFbAttributeSet_AttributeNotRuntimeChangeable,
    // unexpected error
    kVIAFBAttributeSet_InternalError
};

// Access FB attributes at an object
class VIAFbAttributeAccess {
public:
    // releases the attribute access object
    VIASTDDECL Release() = 0;

    // Retrieves the value of an attribute.
    // Returns kVIA_OK, kVIA_DBObjectNotFound (if attribute ID is invalid or no value is set for the attribute),
    // kVIA_WrongSignalType (if attribute definition has a different data type)
    // [IN] attributeID -- ID of the attribute, get it from the VIAFbVIAService if necessary
    // [IN] mode -- retrieval mode, see above
    // [OUT] value -- retrieved value
    VIASTDDECL GetAttributeValueInt32(uint64 attributeID, VIAFbAttributeRetrievalMode mode, int32 &value) const = 0;
    // Retrieves the value of an attribute.
    // Returns kVIA_OK, kVIA_DBObjectNotFound (if attribute ID is invalid or no value is set for the attribute),
    // kVIA_WrongSignalType (if attribute definition has a different data type)
    // [IN] attributeID -- ID of the attribute, get it from the VIAFbVIAService if necessary
    // [IN] mode -- retrieval mode, see above
    // [OUT] value -- retrieved value
    VIASTDDECL GetAttributeValueUInt32(uint64 attributeID, VIAFbAttributeRetrievalMode mode, uint32 &value) const = 0;
    // Retrieves the value of an attribute.
    // Returns kVIA_OK, kVIA_DBObjectNotFound (if attribute ID is invalid or no value is set for the attribute),
    // kVIA_WrongSignalType (if attribute definition has a different data type)
    // [IN] attributeID -- ID of the attribute, get it from the VIAFbVIAService if necessary
    // [IN] mode -- retrieval mode, see above
    // [OUT] value -- retrieved value
    VIASTDDECL GetAttributeValueInt64(uint64 attributeID, VIAFbAttributeRetrievalMode mode, int64 &value) const = 0;
    // Retrieves the value of an attribute.
    // Returns kVIA_OK, kVIA_DBObjectNotFound (if attribute ID is invalid or no value is set for the attribute),
    // kVIA_WrongSignalType (if attribute definition has a different data type)
    // [IN] attributeID -- ID of the attribute, get it from the VIAFbVIAService if necessary
    // [IN] mode -- retrieval mode, see above
    // [OUT] value -- retrieved value
    VIASTDDECL GetAttributeValueUInt64(uint64 attributeID, VIAFbAttributeRetrievalMode mode, uint64 &value) const = 0;
    // Retrieves the value of an attribute.
    // Returns kVIA_OK, kVIA_DBObjectNotFound (if attribute ID is invalid or no value is set for the attribute),
    // kVIA_WrongSignalType (if attribute definition has a different data type)
    // [IN] attributeID -- ID of the attribute, get it from the VIAFbVIAService if necessary
    // [IN] mode -- retrieval mode, see above
    // [OUT] value -- retrieved value
    VIASTDDECL GetAttributeValueDouble(uint64 attributeID, VIAFbAttributeRetrievalMode mode, double &value) const = 0;
    // Retrieves the value of an attribute.
    // Returns kVIA_OK, kVIA_DBObjectNotFound (if attribute ID is invalid or no value is set for the attribute),
    // kVIA_WrongSignalType (if attribute definition has a different data type), kVIA_BufferToSmall
    // [IN] attributeID -- ID of the attribute, get it from the VIAFbVIAService if necessary
    // [IN] mode -- retrieval mode, see above
    // [OUT] valueBuffer -- retrieved value, UTF-8 encoded
    // [IN] bufferSize -- size of the valueBuffer in bytes
    VIASTDDECL GetAttributeValueString(uint64 attributeID, VIAFbAttributeRetrievalMode mode, char *valueBuffer,
                                       size_t bufferSize) const = 0;

    // Sets the value of an attribute.
    // Returns kVIA_OK if successful, kVIA_Failed otherwise; then look at the result parameter.
    // [IN] attributeID -- ID of the attribute, get it from the VIAFbVIAService if necessary
    // [IN] value -- new value for the attribute
    // [OUT] result -- result of the operation
    VIASTDDECL SetAttributeValueInt32(uint64 attributeID, int32 value, VIAFbAttributeSetResult &result) = 0;
    // Sets the value of an attribute.
    // Returns kVIA_OK if successful, kVIA_Failed otherwise; then look at the result parameter.
    // [IN] attributeID -- ID of the attribute, get it from the VIAFbVIAService if necessary
    // [IN] value -- new value for the attribute
    // [OUT] result -- result of the operation
    VIASTDDECL SetAttributeValueUInt32(uint64 attributeID, uint32 value, VIAFbAttributeSetResult &result) = 0;
    // Sets the value of an attribute.
    // Returns kVIA_OK if successful, kVIA_Failed otherwise; then look at the result parameter.
    // [IN] attributeID -- ID of the attribute, get it from the VIAFbVIAService if necessary
    // [IN] value -- new value for the attribute
    // [OUT] result -- result of the operation
    VIASTDDECL SetAttributeValueInt64(uint64 attributeID, int64 value, VIAFbAttributeSetResult &result) = 0;
    // Sets the value of an attribute.
    // Returns kVIA_OK if successful, kVIA_Failed otherwise; then look at the result parameter.
    // [IN] attributeID -- ID of the attribute, get it from the VIAFbVIAService if necessary
    // [IN] value -- new value for the attribute
    // [OUT] result -- result of the operation
    VIASTDDECL SetAttributeValueUInt64(uint64 attributeID, uint64 value, VIAFbAttributeSetResult &result) = 0;
    // Sets the value of an attribute.
    // Returns kVIA_OK if successful, kVIA_Failed otherwise; then look at the result parameter.
    // [IN] attributeID -- ID of the attribute, get it from the VIAFbVIAService if necessary
    // [IN] value -- new value for the attribute
    // [OUT] result -- result of the operation
    VIASTDDECL SetAttributeValueDouble(uint64 attributeID, double value, VIAFbAttributeSetResult &result) = 0;
    // Sets the value of an attribute.
    // Returns kVIA_OK if successful, kVIA_Failed otherwise; then look at the result parameter.
    // [IN] attributeID -- ID of the attribute, get it from the VIAFbVIAService if necessary
    // [IN] value -- new value for the attribute (UTF-8 encoded)
    // [OUT] result -- result of the operation
    VIASTDDECL SetAttributeValueString(uint64 attributeID, const char *value, VIAFbAttributeSetResult &result) = 0;

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 16)
    // Removes a locally set attribute value.
    // Returns kVIA_OK if successful, kVIA_Failed otherwise; then look at the result parameter.
    // [IN] attributeID -- ID of the attribute, get it from the VIAFbVIAService if necessary
    // [OUT] result -- result of the operation
    VIASTDDECL RemoveAttributeValue(uint64 attributeID, VIAFbAttributeSetResult &result) = 0;

#endif
};

enum VIAFbAttributableType {
    kVIAFbAttributable_DO = 0,
    kVIAFbAttributable_DOReference,
    kVIAFbAttributable_DOInterface,
    kVIAFbAttributable_DOVirtualNetwork,
    kVIAFbAttributable_DOContainerDefinition,
    kVIAFbAttributable_DOInstanceContainer,
    kVIAFbAttributable_DOReferenceContainer,
    kVIAFbAttributable_CO,
    kVIAFbAttributable_FEP,
    kVIAFbAttributable_MP,
    kVIAFbAttributable_ServiceInterface,
    kVIAFbAttributable_PDUDefinition,
    kVIAFbAttributable_Port,
    kVIAFbAttributable_Participant,
};

#endif

enum VIAFbDOCreationError {
    kVIAFbDOCreationError_InvalidName,
    kVIAFbDOCreationError_InvalidNamespace,
    kVIAFbDOCreationError_NameNotUnique,
    kVIAFbDOCreationError_NamespaceNotUserExtensible,
    kVIAFbDOCreationError_InvalidAttribute,
    kVIAFbDOCreationError_Other
};

// -------------------------------------------------------------------
// VIA FunctionBus: Factory service

// class VIAFunctionBusService: This class provides an interface for querying objects of the FunctionBus.
// Note: you should use the generic GetValuePort to access measurement point values.
class VIAFbViaService {
public:
    // Release the service, same as calling ReleaseFunctionBusService
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Obtain access to a port sending a signal.
    // Note: this method can also access event provider ports, but the use is restricted to signal sender functionality
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetSignalSenderPort(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbSignalSenderPort **outPort) const = 0;                 // [OUT] Port handle, if successful

    // Obtain access to a port receiving a signal.
    // Note: this method can also access event consumer ports, but the use is restricted to signal receiver functionality
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetSignalReceiverPort(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbSignalReceiverPort **outPort) const = 0;               // [OUT] Port handle, if successful

    // Obtain access to a port consuming a service or free function.
    // Path must refer to a specific port of a service or free *function*, e.g. someNs::someObj[someClient,someServer].someFunc"
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetFunctionClientPort(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbFunctionClientPort **outPort) const = 0;               // [OUT] Port handle, if successful

    // Obtain access to a port providing a service or free function.
    // Path must refer to a specific port of a service or free *function*, e.g. someNs::someObj[someClient,someServer].someFunc"
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetFunctionServerPort(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbFunctionServerPort **outPort) const = 0;               // [OUT] Port handle, if successful

    // Obtain access to a port sending a PDU.
    // Note: this method can also access service PDU sender ports, but the use is restricted to PDU sender functionality
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetPDUSenderPort(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbPDUSenderPort **outPort) const = 0;                    // [OUT] Port handle, if successful

    // Obtain access to a port receiving a PDU.
    // Note: this method can also access service PDU receiver ports, but the use is restricted to PDU receiver functionality
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetPDUReceiverPort(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbPDUReceiverPort **outPort) const = 0;                  // [OUT] Port handle, if successful

    // Obtain access to a service communication object.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if service does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetServiceCO(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbServiceCO **outService) const = 0;                     // [OUT] Service handle, if successful

    // Obtain access to a consumer side end point of a service.
    // Note: service consumers can as well be obtained by name from a service
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if service consumer does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetServiceConsumer(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbServiceConsumer **outConsumer) const = 0;              // [OUT] Service consumer handle, if successful

    // Obtain access to a provider side end point of a service.
    // Note: service providers can as well be obtained by name from a service
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if service provider does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetServiceProvider(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbServiceProvider **outProvider) const = 0;              // [OUT] Service consumer handle, if successful

    // Obtain access to a consumer side service port.
    // Note: consumed service ports can as well be obtained by name from a service
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if consumed service port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetConsumedService(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbConsumedService **outConsumerPort) const = 0;          // [OUT] Consumed service port handle, if successful

    // Obtain access to a provider side service port.
    // Note: provided service ports can as well be obtained by name from a service
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if provided service port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetProvidedService(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbProvidedService **outProviderPort) const = 0;          // [OUT] Provided service port handle, if successful

    // Obtain access to a provider side event end point.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if end point does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetEventProvider(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbEventProvider **outProvider) const = 0;                // [OUT] Event end point handle, if successful

    // Obtain access to a consumer side event port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetConsumedEvent(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbConsumedEvent **outPort) const = 0;                    // [OUT] Event consumer side port handle, if successful

    // Obtain access to a provider side event port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetProvidedEvent(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbProvidedEvent **outPort) const = 0;                    // [OUT] Event provider side port handle, if successful

    // Obtain access to a provider side service PDU end point.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if end point does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetServicePDUProvider(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbServicePDUProvider **outProvider) const = 0;           // [OUT] Service PDU end point handle, if successful

    // Obtain access to a consumer side service PDU port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetConsumedServicePDU(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbConsumedServicePDU **outPort) const = 0;               // [OUT] Service PDU consumer side port handle, if successful

    // Obtain access to a provider side service PDU port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetProvidedServicePDU(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbProvidedServicePDU **outPort) const = 0;               // [OUT] Service PDU provider side port handle, if successful

    // Obtain access to Abstract binding functionality.
    // The binding access object returned is stateless and thus does not need to be released.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL GetAbstractBinding(
            VIAFbAbstractBinding **outBinding) const = 0;               // [OUT] Abstract binding API

    // Obtain access to SOME/IP binding functionality.
    // The binding access object returned is stateless and thus does not need to be released.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL GetSomeIPBinding(
            VIAFbSomeIPBinding **outBinding) const = 0;                 // [OUT] SOME/IP binding API

    // Obtain access to a port generically for reading and writing values.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetValuePort(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbValuePort **outPort) const = 0;                        // [OUT] Port handle, if successful

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 9)
    // Obtain access to a provider side field end point.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if end point does not exist, kVIA_ObjectInvalid if field has no notification
    // [THREADSAFE] Yes
    VIASTDDECL GetFieldProvider(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbFieldProvider **outProvider) const = 0;                // [OUT] Field end point handle, if successful

    // Obtain access to a consumer side field port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist, kVIA_ObjectInvalid if field has no notification
    // [THREADSAFE] Yes
    VIASTDDECL GetConsumedField(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbConsumedField **outPort) const = 0;                    // [OUT] Field consumer side port handle, if successful

    // Obtain access to a provider side field port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist, kVIA_ObjectInvalid if field has no notification
    // [THREADSAFE] Yes
    VIASTDDECL GetProvidedField(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbProvidedField **outPort) const = 0;                    // [OUT] Field provider side port handle, if successful
#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 11)
    // Dynamic endpoint for publishing/subscribing to a particular Distributed Object.
    // Path must reference a particular DO instance, e.g. "someNs::someObjectList[2]", "someNs::someObjectMap["endpoint1"]"
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist, kVIA_ObjectInvalid if DO instance does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetDO(
            const char *inPath,
            VIAFbDO **outDistributedObject) const = 0;

    // Dynamic endpoint for publishing/subscribing to a particular Distributed Object.
    // Note: Creating new collections is not supported by this method. But instantiating DOs and adding them to existing collections is possible, with inName set to
    //       * "someNs::someDynamicList" (append to list),
    //       * "someNs::someDynamicArray[3]" (insert into array),
    //       * "someNs::someDynamicMap["xyz"]" (insert into map).
    // [OUT] kVIA_OK if the instantiation was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if DO interface does not exist, kVIA_ObjectInvalid if DO instance does already exist
    // [THREADSAFE] Yes
    VIASTDDECL CreateDO(
            const char *inPath,                                         // [IN] Path must reference a particular DO type, e.g. "someNs::someInterface"
            const char *inName,                                         // [IN] DO instance name that does not exist yet and is to be defined, e.g. "someNs::someDynamicObject"
            VIAFbDO **outDistributedObject) const = 0;

    // Retrieves an attribute ID from its full path.
    // Return kVIA_OK or kVIA_DBAttributeNameNotFound
    // [IN] attribute path
    // [OUT] attribute ID
    VIASTDDECL GetAttributeID(const char *inPath, uint64 &outID) const = 0;

    // Creates an attribute access object to get or set attributes of an FB object.
    // Returns kVIA_OK or kVIA_DBObjectNotFound
    // [IN] type of the object
    // [IN] full path of the object
    // [OUT] attribute access or nullptr
    VIASTDDECL CreateAttributeAccess(VIAFbAttributableType attributableType, const char *inPath,
                                     VIAFbAttributeAccess **outAttributeAccess) const = 0;

    // Creates an attribute access object to get or set attributes of an FB object.
    // Returns kVIA_OK or kVIA_DBObjectNotFound
    // [IN] type of the object
    // [IN] ID of the object
    // [OUT] attribute access or nullptr
    VIASTDDECL CreateAttributeAccess(VIAFbAttributableType attributableType, uint64 inId,
                                     VIAFbAttributeAccess **outAttributeAccess) const = 0;

    // Retrieves the virtual DO network with the specified ID.
    // Returns kVIA_OK or kVIA_DBObjectNotFound or kVIA_ParameterInvalid (if out pointer is nullptr)
    // [IN] network ID
    // [OUT] network
    VIASTDDECL GetDONetwork(uint64 networkID, VIAFbDONetwork **network) const = 0;

    // Notifies CANoe that a binding error has occurred. This will be shown to the user through a special value entity.
    // All strings must be UTF-8 encoded.
    // Can only be called on threads with a runtime environment (DO Server, VE Server).
    // Always returns kVIA_OK.
    // [IN] error code
    // [IN] sub-error code (optional, may be nullptr)
    // [IN] binding type name (e.g. "MQTT")
    // [IN] error message
    // [IN] path to the FB object where the binding error occurred (optional, may be nullptr)
    VIASTDDECL BindingErrorOccurred(int32 errorCode, const int32 *subErrorCode, const char *bindingBlockName,
                                    const char *errorMessage, const char *objectPath) const = 0;

#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 13)
    // create a custom binding for a DO member
    // ownership of the member is not passed to the new binding
    VIASTDDECL CreateCustomBinding(VIAFbDOMember *pMember, VIAFbDOBinding **outBinding) = 0;
    // remove a single DO member custom binding.
    VIASTDDECL RemoveCustomBinding(VIAFbDOBinding *pBinding) = 0;

#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 16)
    // Returns the value type of an attribute. This can only be signed / unsigned integer,
    // float, or string.
    // Returns kVIA_OK or kVIA_DBObjectNotFound
    // [IN] attribute ID (retrieved with a call to GetAttributeID)
    // [OUT] value type of the attribute
    // [OUT} value size in case of integer values: 32 or 64
    VIASTDDECL GetAttributeValueType(uint64 attributeId, VIAFbTypeTag *outType, int32 *outValueSize) = 0;

    // Retrieve a distributed object by its ID.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if DO was not found
    // [THREADSAFE] Yes
    VIASTDDECL GetDOByID(
            uint64 inID,
            VIAFbDO **outDistributedObject) const = 0;

    // Retrieve a distributed object interface
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if the interface was not found
    // [THREADSAFE] Yes
    VIASTDDECL GetDOInterface(const char *inPath, VIAFbDOInterface **outDOInterface) const = 0;
    // Retrieve a distributed object interface by its ID.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if the interface was not found
    // [THREADSAFE] Yes
    VIASTDDECL GetDOInterfaceByID(uint64 inID, VIAFbDOInterface **outDOInterface) const = 0;

    // Create a new distributed object (which is not part of a container).
    // [IN] full path of the DO namespace. Namespaces are also created as necessary if the path or parts of it don't exist yet.
    // [IN] name of the new object
    // [IN] blueprint for the object. Note: this is not optional, a blueprint must be given.
    // [OUT] reference to the new DO
    // [OUT] error code if return is kVIA_Failed
    // [RETURN] kVIA_OK; kVIA_Failed if object could not be created (e.g. wrong name, duplicate name etc.); kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL CreateDistributedObject(const char *inNamespacePath, const char *inName,
                                       const VIAFbDOBlueprint *inBlueprint, VIAFbDO **outDO,
                                       VIAFbDOCreationError *outErrorCode) = 0;
    // Delete a distributed object which was dynamically created and which is not part of a container.
    // [IN] full path of the DO
    // [RETURN] kVIA_OK; kVIA_Failed if object could not be deleted (e.g. not a dynamic DO); kVIA_ObjectNotFound if object wasn't found;
    // kVIA_ParameterInvalid in case of a bad parameter
    VIASTDDECL DeleteDistributedObject(const char *inPath) = 0;
    // Delete a distributed object which was dynamically created and which is not part of a container.
    // [IN] full path of the DO
    // [RETURN] kVIA_OK; kVIA_Failed if object could not be deleted (e.g. not a dynamic DO);
    // kVIA_ParameterInvalid in case of a bad parameter
    VIASTDDECL DeleteDistributedObjectByID(uint64 inID) = 0;

    // Retrieve a distributed object container.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if the container was not found
    // [THREADSAFE] Yes
    VIASTDDECL GetDOContainer(const char *inPath, VIAFbDOContainer **outDOContainer) const = 0;

#endif
};

// -------------------------------------------------------------------
// VIA FunctionBus: Generic value ports (signal, PDU, service FEP, ...)

class VIAFbValuePort {
public:
    // Release the port instance, must be called for any port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve the value of this port that was last set.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member if this is a composite type
            VIAFbValueClass inValueClass,                               // [IN] Identifies the specific value entity associated with the port
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object, if successful

    // Obtain access to a FunctionBus data type for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] Yes
    VIASTDDECL GetType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbValueClass inValueClass,                               // [IN] Identifies the specific value entity associated with the port
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Update the value of the port. Depending on the port and value class this may fail (read-only values).
    // [OUT] kVIA_OK if the update was successful, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle
    // [THREADSAFE] No
    VIASTDDECL SetValue(
            const VIAFbValue *inValue) = 0;                             // [IN] The value object containing the new value

    // Retrieve status information of this port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetStatus(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member if this is a composite type
            VIAFbValueClass inValueClass,                               // [IN] Identifies the specific value entity associated with the port
            VIAFbStatus **outStatus) const = 0;                         // [OUT] The status object, if successful

    // Retrieve the statically configured initialization value of this port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetInitValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this initial value is accessed
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member if this is a composite type
            VIAFbValueClass inValueClass,                               // [IN] Identifies the specific value entity associated with the port
            VIAFbValue **outInitValue) const = 0;                       // [OUT] The initial value object, if successful

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 10)
    // Retrieve the PDU signal value of this port that was last set.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPDUSignalValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            const char *inSignalName,                                   // [IN] Identifies the specific PDU signal associated with the port
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member if this is a composite type
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object, if successful

    // Obtain access to a FunctionBus data type of a PDU signal for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] Yes
    VIASTDDECL GetPDUSignalType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            const char *inSignalName,                                   // [IN] Identifies the specific PDU signal associated with the port
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful
#endif
};

// -------------------------------------------------------------------
// VIA FunctionBus: Signal sender/receiver ports

class VIAFbSignalSenderPort {
public:
    // Release the port instance, must be called for any port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve the value of this signal sender port that was last set.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member if this is a composite type
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object, if successful

    // Obtain access to a FunctionBus data type for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] Yes
    VIASTDDECL GetType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Update the value of a signal's sender port to be send. Depending on the signal port's configuration,
    // the updated value will be sent either immediately or with the upcoming cycle.
    // [OUT] kVIA_OK if the update was successful, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle
    // [THREADSAFE] No
    VIASTDDECL SetValue(
            const VIAFbValue *inValue) = 0;                             // [IN] The value object containing the new value

    // Retrieve status information of this port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetStatus(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member if this is a composite type
            VIAFbStatus **outStatus) const = 0;                         // [OUT] The status object, if successful

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

class VIAFbSignalReceiverPort {
public:
    // Release the port instance, must be called for any port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve the value of this signal receiver port that was last received.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member if this is a composite type
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object, if successful

    // Obtain access to the signal's FunctionBus data type for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] No
    VIASTDDECL GetType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Retrieve status information of this port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetStatus(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member if this is a composite type
            VIAFbStatus **outStatus) const = 0;                         // [OUT] The status object, if successful

    // Register an observer to be notified when this signal port is updated, or the updated value changes.
    // The time the value is received depends on the port's binding properties (e.g., cyclic, immediately).
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbSignalReceiverObserver *inObserver,                    // [IN] Pointer to an object implementing the observer template method
            VIAFbUpdateMode inUpdateMode,                               // [IN] Decide to observe only changes in value or any value updates
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

// -------------------------------------------------------------------
// VIA FunctionBus: Event sender/receiver ports and provider FEP

class VIAFbConsumedEvent : public VIAFbSignalReceiverPort {
public:
    // Release the port instance, must be called for any port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve the value entity representing the subscription state of the consumer side event port.
    // [OUT] kVIA_OK if subscription state is available, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetSubscriptionState(
            VIAFbSubscriptionState *outState) const = 0;                // [OUT] The subscription state, if successful

    // Modifies the port local subscription state of the event without any network interaction.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL SetSubscriptionStateIsolated(
            VIAFbSubscriptionState inState) = 0;                        // [IN] The requested subscription state

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

class VIAFbProvidedEvent : public VIAFbSignalSenderPort {
public:
    // Release the port instance, must be called for any port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve the value entity representing the subscription state of the provider side event port.
    // [OUT] kVIA_OK if subscription state is available, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetSubscriptionState(
            VIAFbSubscriptionState *outState) const = 0;                // [OUT] The value object, if successful

    // Modifies the port local subscription state of the event without any network interaction.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL SetSubscriptionStateIsolated(
            VIAFbSubscriptionState inState) = 0;                        // [IN] The requested subscription state

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

class VIAFbEventProvider {
public:
    // Release the provider instance, must be called for any provider instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve an iterator over all service consumers currently subscribed at the event provider
    // [OUT] kVIA_OK if subscribed consumers could be determined, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetSubscribedConsumers(
            VIAFbServiceConsumerIterator **outIterator) = 0;            // [OUT] An iterator over the currently subscribed consumers

    // Retrieve the latest model value of this event provider.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object, if successful

    // Update the model value of an event provider.
    // [OUT] kVIA_OK if the update was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL SetValue(
            const VIAFbValue *inValue) = 0;                             // [IN] The value object containing the new event value

    // Obtain access to a FunctionBus data type for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] Yes
    VIASTDDECL GetType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Retrieve status information of this event provider's value entity.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetStatus(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member if this is a composite type
            VIAFbStatus **outStatus) const = 0;                         // [OUT] The status object, if successful

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

// -------------------------------------------------------------------
// VIA FunctionBus: PDU sender/receiver ports

class VIAFbPDUSenderPort {
public:
    // Release the port instance, must be called for any port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve the latest value of this PDU sender port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object, if successful

    // Update the value of a PDU's sender port to be sent. Depending on the PDU port's configuration,
    // the updated value will be sent either immediately or with the upcoming cycle.
    // [OUT] kVIA_OK if the update was successful, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle
    // [THREADSAFE] No
    VIASTDDECL SetValue(
            const VIAFbValue *inValue) = 0;                             // [IN] The value object containing the new value

    // Retrieve status information of this port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetStatus(
            VIAFbStatus **outStatus) const = 0;                         // [OUT] The status object, if successful

    // Retrieve the value of a signal mapped into this PDU.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalue in case of a bad parameter, kVIA_ObjectNotFound if the signal does not exist, kVIA_Failed in case of an error
    VIASTDDECL GetSignalValue(
            const char *inSignalName,                                   // [IN] Name of the mapped signal
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object containing the new value

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

class VIAFbPDUReceiverPort {
public:
    // Release the port instance, must be called for any port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve the latest received value of this PDU receiver port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object, if successful

    // Retrieve status information of this port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetStatus(
            VIAFbStatus **outStatus) const = 0;                         // [OUT] The status object, if successful

    // Retrieve the value of a signal mapped into this PDU.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalue in case of a bad parameter, kVIA_ObjectNotFound if the signal does not exist, kVIA_Failed in case of an error
    VIASTDDECL GetSignalValue(
            const char *inSignalName,                                   // [IN] Name of the mapped signal
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object containing the new value

    // Register an observer to be notified when this PDU port is updated, or the updated value changes.
    // The time the value is received depends on the port's binding properties (e.g., cyclic, immediately).
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbPDUReceiverObserver *inObserver,                       // [IN] Pointer to an object implementing the observer template method
            VIAFbUpdateMode inUpdateMode,                               // [IN] Decide to observe only changes in value or any value updates
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

// -------------------------------------------------------------------
// VIA FunctionBus: Service PDU sender/receiver ports and provider FEP

class VIAFbConsumedServicePDU : public VIAFbPDUReceiverPort {
public:
    // Release the port instance, must be called for any port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve the value entity representing the subscription state of the consumer side PDU port.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL GetSubscriptionState(
            VIAFbSubscriptionState *outState) const = 0;                // [OUT] The current state, if successful

    // Modifies the port local subscription state of the PDU without any network interaction.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL SetSubscriptionStateIsolated(
            VIAFbSubscriptionState inState) = 0;                        // [IN] The requested subscription state

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

class VIAFbProvidedServicePDU : public VIAFbPDUSenderPort {
public:
    // Release the port instance, must be called for any port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve the value entity representing the subscription state of the provider side PDU port.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL GetSubscriptionState(
            VIAFbSubscriptionState *outState) const = 0;                // [OUT] The value object, if successful

    // Modifies the port local subscription state of the PDU without any network interaction.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL SetSubscriptionStateIsolated(
            VIAFbSubscriptionState inState) = 0;                        // [IN] The requested subscription state

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

class VIAFbServicePDUProvider {
public:
    // Release the provider instance, must be called for any provider instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve an iterator over all service consumers currently subscribed at the PDU provider
    // [OUT] kVIA_OK if subscribed consumers could be determined, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetSubscribedConsumers(
            VIAFbServiceConsumerIterator **outIterator) = 0;            // [OUT] An iterator over the currently subscribed consumers

    // Retrieve the latest model value of this PDU provider.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object, if successful

    // Update the model value of an PDU provider.
    // [OUT] kVIA_OK if the update was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL SetValue(
            const VIAFbValue *inValue) = 0;                             // [IN] The value object containing the new PDU value

    // Obtain access to a FunctionBus data type for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] Yes
    VIASTDDECL GetType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Retrieve status information of this port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetStatus(
            VIAFbStatus **outStatus) const = 0;                         // [OUT] The status object, if successful

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback

    // Retrieve the value of a signal mapped into this PDU.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalue in case of a bad parameter, kVIA_ObjectNotFound if the signal does not exist, kVIA_Failed in case of an error
    VIASTDDECL GetSignalValue(
            const char *inSignalName,                                   // [IN] Name of the mapped signal
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object containing the new value
#endif
};

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 9)
// -------------------------------------------------------------------
// VIA FunctionBus: Field sender/receiver ports and provider FEP

class VIAFbConsumedField : public VIAFbSignalReceiverPort {
public:
    // Release the port instance, must be called for any port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve the value entity representing the subscription state of the consumer side field port.
    // [OUT] kVIA_OK if subscription state is available, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetSubscriptionState(
            VIAFbSubscriptionState *outState) const = 0;                // [OUT] The subscription state, if successful

    // Modifies the port local subscription state of the field without any network interaction.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL SetSubscriptionStateIsolated(
            VIAFbSubscriptionState inState) = 0;                        // [IN] The requested subscription state

    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
};

class VIAFbProvidedField : public VIAFbSignalSenderPort {
public:
    // Release the port instance, must be called for any port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve the value entity representing the subscription state of the provider side field port.
    // [OUT] kVIA_OK if subscription state is available, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetSubscriptionState(
            VIAFbSubscriptionState *outState) const = 0;                // [OUT] The value object, if successful

    // Modifies the port local subscription state of the field without any network interaction.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL SetSubscriptionStateIsolated(
            VIAFbSubscriptionState inState) = 0;                        // [IN] The requested subscription state

    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
};

class VIAFbFieldProvider {
public:
    // Release the provider instance, must be called for any provider instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve an iterator over all service consumers currently subscribed at the field provider
    // [OUT] kVIA_OK if subscribed consumers could be determined, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetSubscribedConsumers(
            VIAFbServiceConsumerIterator **outIterator) = 0;            // [OUT] An iterator over the currently subscribed consumers

    // Retrieve the latest model value of this field provider.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object, if successful

    // Update the model value of a field provider.
    // [OUT] kVIA_OK if the update was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL SetValue(
            const VIAFbValue *inValue) = 0;                             // [IN] The value object containing the new field value

    // Obtain access to a FunctionBus data type for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] Yes
    VIASTDDECL GetType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Retrieve status information of this field provider's value entity.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetStatus(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member if this is a composite type
            VIAFbStatus **outStatus) const = 0;                         // [OUT] The status object, if successful

    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
};

#endif

// -------------------------------------------------------------------
// VIA FunctionBus: Distributed Object access
#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 11)

// Base interface for all specialized DO member interfaces
class VIAFbDOMemberInterface {
public:
    // Release the interface instance, must be called once for any instance obtained from the member.
    // [RETURN] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() const = 0;

    // Get a specialized member interface
    // The returned object can be casted to the desired interface if kVIA_OK is returned.
    // You have to call Release on the returned object then.
    // [IN] the desired interface type
    // [OUT] the interface or nullptr
    // [RETURN] kVIA_OK if successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_MissingInterface if the member
    // doesn't provide the desired interface.
    // [THREADSAFE] No
    VIASTDDECL GetMemberInterface(VIAFbDOMemberInterfaceType inInterfaceType,
                                  VIAFbDOMemberInterface **outInterface) = 0;
};

// Helper function to retrieve a specific member interface
template<typename T>
inline VIAResult VIAQueryFbDOMemberInterface(VIAFbDOMemberInterface *inInterfaceType, T **outInterfaceType) {
    if (inInterfaceType == nullptr) return kVIA_ParameterInvalid;
    if (outInterfaceType == nullptr) return kVIA_ParameterInvalid;
    VIAFbDOMemberInterface *outIF = nullptr;
    VIAResult result = inInterfaceType->GetMemberInterface(
            static_cast<VIAFbDOMemberInterfaceType>(T::InterfaceType), &outIF);
    if (result != kVIA_OK) return result;
    *outInterfaceType = static_cast<T *>(outIF);
    return result;
}

template<typename T>
inline VIAResult
VIAQueryConstFbDOMemberInterface(const VIAFbDOMemberInterface *inInterfaceType, const T **outInterfaceType) {
    if (outInterfaceType == nullptr) return kVIA_ParameterInvalid;
    T *nonConstResult = nullptr;
    VIAResult ret = VIAQueryFbDOMemberInterface(const_cast<VIAFbDOMemberInterface *>(inInterfaceType), &nonConstResult);
    *outInterfaceType = nonConstResult;
    return ret;
}


// Callback handler to notify observers whenever a DO member receives a value.
class VIAFbDOMemberValueObserver {
public:
    // Indicates that the signal value at the receiver port has changed
    // [OUT] Return value is ignored
    VIASTDDECL OnValue(
            VIATime inTime,                      // [IN] Absolute time the signal value was updated
            const VIAFbDOMember *inMember) = 0;  // [IN] Receiving DO member
};


class VIAFbDO {
public:
    // Release the DO's instance, must be called for any instance obtained from the VIA service to free resources after use.
    // [RETURN] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve an iterator over all members of this distributed object.
    // [OUT] iterator over the members
    // [RETURN] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetMembers(
            VIAFbMemberIterator<VIAFbDOMember> **outIterator) = 0;

    // Retrieve an iterator over all members of this distributed object which provide a specific member interface.
    // The objects returned by the iterator can be casted to the desired interface.
    // The interface design is such that always:
    // - a provided member provides the generic interface, the provider interface and the provided interface
    // - a consumed member provides the generic interface, the consumer interface and the consumed interface
    // - an internal member provides the generic interface, the provider interface, the consumer interface and the internal interface.
    // For example, a provided method member provides VIAFbDOMethod, VIAFbDOMethodProvider and VIAFbDOProvidedMethod
    // a consumed method member provides VIAFbDOMethod, VIAFbDOMethodConsumer and VIAFbDOConsumedMethod
    // an internal method member provides VIAFbDOMethod, VIAFbDOMethodConsumer, VIAFbDOMethodProvider and VIAFbDOInternalMethod
    // This design ensures that the specific interfaces can be extended later without breaking binary compatibility, since there
    // is no class inheritance.
    // [IN] the desired interface type
    // [OUT] iterator over the members
    // [RETURN] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetMembersWithInterface(
            VIAFbDOMemberInterfaceType inInterfaceType,
            VIAFbMemberIterator<VIAFbDOMemberInterface> **outIterator) = 0;

    // Retrieve a specific member.
    // [IN] the relative path to the member, without a leading '.'
    // [OUT] the member
    // [RETURN] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetMemberByPath(const char *relMemberPath, VIAFbDOMember **outMember) = 0;

    // Access to attributes.
    // Returns kVIA_OK or kVIA_ParameterInvalid (if given pointer is nullptr)
    // [OUT] Pointer receiving the attribute access object. The object must be
    // deleted by calling Release on it.
    // [RETURN] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] Yes
    VIASTDDECL GetAttributeAccess(VIAFbAttributeAccess **outAttributeAccess) = 0;

    // Retrieve the direct base interface of this distributed object.
    // [OUT] the interface
    // [RETURN] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetBaseDOInterface(
            VIAFbDOInterface **outDOInterface) = 0;

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 16)
    // Retrieve a unique ID for this DO
    // [RETURN] kVIA_OK if successful, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetID(uint64 *outID) const = 0;

#endif
};

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 16)

// A blueprint to create a DO dynamically.
// The object can either represent a blueprint for the whole DO, or the same interface is also
// used to set attributes and virtual networks for members of the DO.
class VIAFbDOBlueprint {
public:
    // Releases the object. If the object represents a top-level blueprint, that blueprint is
    // also destroyed.
    VIASTDDECL Release() = 0;
    // Returns the ID of the interface of the blueprint. For a top-level blueprint or its simple
    // members, this is the top-level interface ID. For embedded member blueprints (or its simple
    // members), it is the ID of the interface of the embedded member.
    // [OUT] interface ID
    // [RETURN] kVIA_OK, kVIA_ParameterInvalid in case of a bad parameter.
    // [THREADSAFE} yes
    VIASTDDECL GetInterfaceID(uint64 *outInterfaceID) const = 0;
    // Access to attributes for the blueprint.
    // The object may provide only write access to the attributes, not read access.
    // Returns kVIA_OK or kVIA_ParameterInvalid (if given pointer is nullptr)
    // [OUT] Pointer receiving the attribute access object. The object must be
    // deleted by calling Release on it.
    // [THREADSAFE] No
    VIASTDDECL GetAttributeAccess(VIAFbAttributeAccess **outAttributeAccess) = 0;
    // Add a virtual network to the blueprint. In case of simple members, it replaces the previously
    // set virtual network, if any.
    // [IN] full path to the network or ID of the network
    // [RETURN] kVIA_OK if the network could be added. kVIA_ParameterInvalid in case of a bad parameter.
    // [THREADSAFE] No
    VIASTDDECL AddVirtualNetwork(const char *networkPath) = 0;

    VIASTDDECL AddVirtualNetwork(uint64 networkID) = 0;
    // Remove a virtual network from the blueprint
    // [IN] full path to the network or ID of the network
    // [RETURN] kVIA_OK if the network could be removed.
    // kVIA_ParameterInvalid in case of a bad parameter.
    // [THREADSAFE] No
    VIASTDDECL RemoveVirtualNetwork(const char *networkPath) = 0;

    VIASTDDECL RemoveVirtualNetwork(uint64 networkID) = 0;
    // Get a blueprint for a member
    // The lifetime of that blueprint is bound to the parent blueprint, even though the VIA object is alive
    // until Release is called.
    // [IN] name of the member
    // [OUT] blueprint for the member
    // [RETURN] kVIA_OK, kVIA_ObjectNotFound if the member was not found, kVIA_ParameterInvalid in case of a bad parameter.
    // [THREADSAFE} No
    VIASTDDECL GetMemberBlueprint(const char *memberName, VIAFbDOBlueprint **outBlueprint) = 0;
    // Removes all attributes and virtual networks for the top-level blueprint and all member blueprints
    VIASTDDECL Clear() = 0;
};

// A container for distributed objects or DO references
class VIAFbDOContainer {
public:
    // Releases the object
    VIASTDDECL Release() = 0;
    // Query whether the leafs of the container structure are DOs or DO references
    // [OUT] true iff the container stores references
    // [RETURN] kVIA_OK or kVIA_ParameterInvalid
    // [THREADSAFE] Yes
    VIASTDDECL IsReferenceContainer(bool *outIsReference) const = 0;
    // Query whether the object represents the last dimension of a container
    // [OUT] true iff the container directly contains DOs / references
    // [RETURN] kVIA_OK or kVIA_ParameterInvalid
    // [THREADSAFE] Yes
    VIASTDDECL IsLeafContainer(bool *outIsLeaf) const = 0;
    // Query the number of dimensions of the container
    // [OUT] the number of dimensions of the container
    // [RETURN] kVIA_OK or kVIA_ParameterInvalid
    // [THREADSAFE] Yes
    VIASTDDECL GetNrOfDimensions(uint32 *outNrOfDimensions) const = 0;
    // Query the current size of the container
    // [OUT] the current size of the container (in elements)
    // [RETURN] kVIA_OK or kVIA_ParameterInvalid
    // [THREADSAFE] Yes
    VIASTDDECL GetCurrentSize(uint32 *outSize) const = 0;
    // Query the minimum size of the container
    // [OUT] the minimum size of the container (in elements)
    // [RETURN] kVIA_OK or kVIA_ParameterInvalid
    // [THREADSAFE] Yes
    VIASTDDECL GetMinimumSize(uint32 *outSize) const = 0;
    // Query the maximum size of the container
    // [OUT] whether the container has a maxmum size set
    // [OUT] the maximum size of the container (in elements), if the container has a maximum; else the parameter is ignored
    // [RETURN] kVIA_OK or kVIA_ParameterInvalid
    // [THREADSAFE] Yes
    VIASTDDECL GetMaximumSize(bool *outHasMaximum, uint32 *outSize) const = 0;

    // Adds an element at the end of the container, enlarging the container
    // [RETURN] kVIA_OK; kVIA_Failed if the container has reached its maximum size
    // [THREADSAFE] No
    VIASTDDECL AppendElement() = 0;
    // Removes the last element of the container, reducing the container size
    // [RETURN] kVIA_OK; kVIA_Failed if the container has reached its minimum size
    // [THREADSAFE] No
    VIASTDDECL RemoveLastElement() = 0;
    // Sets a new size of the container. Existing elements are unchanged if the container is made larger; elements at the
    // end are removed if the container is made smaller.
    // [RETURN] kVIA_OK; kVIA_Failed if the size is not in the interval [minimumSize, maximumSize]
    // [THREADSAFE] No
    VIASTDDECL Resize(uint32 newSize) = 0;

    // For leaf containers of DOs: adds an element at the end of the container, enlarging the container
    // [IN] Blueprint for the DO (optional, if nullptr is passed, the container blueprint will be used)
    // [RETURN] kVIA_OK; kVIA_Failed if the container has reached its maximum size; kVIA_ParameterInvalid if blueprint is invalid
    // [THREADSAFE] No
    VIASTDDECL AppendDOElement(VIAFbDOBlueprint *blueprint) = 0;
    // For leaf containers of DOs: destroys the element a the specified index
    // [IN] index of the DO
    // [RETURN] kVIA_OK; kVIA_Failed if index is invalid
    // [THREADSAFE] No
    VIASTDDECL DestroyDOElement(uint32 index) = 0;
    // For leaf containers of DOs: recreates the element at the specified index. If there is already an element at the index,
    // it is destroyed first.
    // [IN] index of the DO
    // [IN] Blueprint for the DO (optional, if nullptr is passed, the container blueprint will be used)
    // [RETURN] kVIA_OK; kVIA_Failed if index is invalid; kVIA_ParameterInvalid if blueprint is invalid
    // [THREADSAFE] No
    VIASTDDECL RecreateDOElement(uint32 index, VIAFbDOBlueprint *blueprint) = 0;

    // For leaf containers of DOs: returns the element at the specified index.
    // [IN] index of the DO
    // [OUT] the DO at the index or null if there is no DO at the index
    // [RETURN] kVIA_OK; kVIA_Failed if index is invalid; kVIA_ParameterInvalid if DO parameter is nullptr
    // [THREADSAFE] No
    VIASTDDECL GetDOElement(uint32 index, VIAFbDO **outDO) const = 0;

    // For non-leaf containers: returns an object for the next dimension, at the specified index
    // [IN] index of the sub-container
    // [OUT} sub-container for the specified index
    // [RETURN] kVIA_OK; kVIA_Failed if index is invalid; kVIA_ParameterInvalid if container parameter is nullptr
    // [THREADSAFE] No
    VIASTDDECL GetSubContainer(uint32 index, VIAFbDOContainer **outContainer) = 0;
};

#endif

class VIAFbDOInterface {
public:
    // Release the DO interface's instance, must be called for any instance obtained from the VIA service to free resources after use.
    // [RETURN] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve the name this DO interface (i.e. the last part of the relative path).
    // [RETURN] kVIA_OK if successful, kVIA_BufferToSmall if buffer too small, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetName(char *inBuffer, size_t inBufferLength) const = 0;

    // Retrieve the full DO interface path (including namespaces)
    // [RETURN] kVIA_OK if successful, kVIA_BufferToSmall if buffer too small, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetFullPath(char *inBuffer, size_t inBufferLength) const = 0;

    // Return whether the DO interface is reverse, based on the DO it is transitively referenced from, and relative to how it was declared.
    // [RETURN] True iff it is (an uneven number of times) reverse, false in any other case
    // [THREADSAFE] No
    VIABOOLDECL IsReverse() const = 0;

    // Retrieve an iterator over all direct base interfaces of this distributed object interface.
    // [OUT] iterator over the interfaces
    // [RETURN] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetBaseDOInterfaces(
            VIAFbInterfaceIterator<VIAFbDOInterface> **outIterator) = 0;

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 16)
    // Access to attributes.
    // Returns kVIA_OK or kVIA_ParameterInvalid (if given pointer is nullptr)
    // [OUT] Pointer receiving the attribute access object. The object must be
    // deleted by calling Release on it.
    // [THREADSAFE] No
    VIASTDDECL GetAttributeAccess(VIAFbAttributeAccess **outAttributeAccess) = 0;

    // Retrieve a unique ID for this DO interface
    // [RETURN] kVIA_OK if successful, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetInterfaceID(uint64 *outID) const = 0;

    // Create a blueprint typed with this DO interface
    // [IN] whether the reverse interface shall be used
    // [RETURN] kVIA_OK if successful, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL CreateBlueprint(bool reverse, VIAFbDOBlueprint **outBlueprint) = 0;

#endif

    //// Retrieve an iterator over all members of this distributed object interface.
    //// [OUT] iterator over the members
    //// [RETURN] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter
    //// [THREADSAFE] No
    //VIASTDDECL GetMembers(
    //  VIAFbMemberIterator<VIAFbDOMember>** outIterator) = 0;
};

// General API common to all kinds of DO instance members
class VIAFbDOMember : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_Generic
    };

    // Retrieve the relative path of this DO member (without a leading '.').
    // [RETURN] kVIA_OK if successful, kVIA_BufferToSmall if buffer too small, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetRelativePath(
            char *inBuffer,                                             // [IN] Pointer to the string buffer to be filled
            size_t inBufferLength) const = 0;                           // [IN] Size in bytes of the string buffer
    // Retrieve the number of characters of the relative path of this DO member (excluding terminating zero)
    // [RETURN] kVIA_OK if query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetRelativePathLength(
            size_t *outLength) const = 0;

    // Get the direction of this DO instance's member, either provided or consumed side.
    // [RETURN] kVIA_OK if query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetDirection(
            VIAFbDOMemberDirection *outDirection) const = 0;

    // Get this DO instance's member type, one of Data, Method, Event, Field.
    // [RETURN] kVIA_OK if query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetMemberType(
            VIAFbDOMemberType *outType) const = 0;

    // Access to attributes.
    // Returns kVIA_OK or kVIA_ParameterInvalid (if given pointer is nullptr)
    // [OUT] Pointer receiving the attribute access object. The object must be
    // deleted by calling Release on it.
    // [THREADSAFE] No
    VIASTDDECL GetAttributeAccess(VIAFbAttributeAccess **outAttributeAccess) = 0;

    // Get the parent member (if there is one)
    // Returns kVIA_OK or kVIA_DBObjectNotFound or kVIA_ParameterInvalid (if given pointer is nullptr)
    // [OUT] Pointer receiving the attribute access object. The object must be
    // deleted by calling Release on it.
    // [THREADSAFE] No
    VIASTDDECL GetParentMember(VIAFbDOMember **outParentMember) = 0;

    // Retrieve the name this DO member (i.e. the last part of the relative path).
    // [RETURN] kVIA_OK if successful, kVIA_BufferToSmall if buffer too small, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetMemberName(char *inBuffer, size_t inBufferLength) const = 0;

    // Retrieve a unique ID for this DO member
    // [RETURN] kVIA_OK if successful, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetUniqueMemberID(uint64 *outUniqueID) const = 0;

    // Retrieve the full member path (including the DO path)
    // [RETURN] kVIA_OK if successful, kVIA_BufferToSmall if buffer too small, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetFullPath(char *inBuffer, size_t inBufferLength) const = 0;

    // Retrieve the distributed object interface from all base interfaces of this distributed object that declares the member.
    // [OUT] The interface
    // [RETURN] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetDOInterface(
            VIAFbDOInterface **outInterface) = 0;

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 16)
    // Retrieve the ID of the DO which contains the member.
    // [OUT] the ID
    // [RETURN] kVIA_OK or kVIA_ParameterInvalid
    // [THREADSAFE] No
    VIASTDDECL GetDOID(uint64 *outDOID) const = 0;

#endif
};

// Callback handler to notify method member observers when the called member receives the call or either an error or the result arrives.
class VIAFbDOMethodObserver {
public:
    // Indicates that the invocation initiated at the member changed its state
    // Note: The obtained inCallContext must be released explicitly via inCallContext->Release.
    // [OUT] Return value is ignored
    VIASTDDECL OnCallStateChanged(
            VIATime inTime,                                    // [IN] Absolute time this state change was observed
            VIAFbCallContext *inCallContext,                   // [IN] Call context of the affected call
            VIAFbFunctionCallState inCallState,                // [IN] State reached
            const VIAFbDOMethod *inMethod) = 0;                // [IN] Method member
};

class VIAFbDOMethod : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_Method
    };

    // Obtain access to the FunctionBus data type of function's input argument for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] No
    VIASTDDECL GetInParamsType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Obtain access to the FunctionBus data type of function's output argument for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] No
    VIASTDDECL GetOutParamsType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Register an observer to be notified when calls handled by this DO member change their state.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbDOMethodObserver *inObserver,                           // [IN] Pointer to an object implementing the observer template method
            VIAFbFunctionCallState inStateToObserve,                    // [IN] The observer is triggered for any call reaching this state
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inCallbackHandle) = 0;            // [IN] Handle from a previously registered observer callback

    // Obtain a value handle containing all input arguments of this DO member's most recent call.
    // [OUT] kVIA_OK if parameters could be retrieved, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetLatestCall(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level the value is accessed
            VIAFbValue **outInParamsValue) const = 0;                   // [OUT] The value object, if successful

    // Obtain a value handle containing all output arguments of this DO member's most recently received response.
    // [OUT] kVIA_OK if parameters could be retrieved, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetLatestReturn(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level the value is accessed
            VIAFbValue **outOutParamsValue) const = 0;                  // [OUT] The value object, if successful

    // Obtain a value interface for the in parameters
    // [OUT] kVIA_OK if parameters could be retrieved, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetInParamsValue(VIAFbDOValue **outInParamsValue) const = 0;

    // Obtain a value interface for the out parameters
    // [OUT] kVIA_OK if parameters could be retrieved, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetOutParamsValue(VIAFbDOValue **outOutParamsValue) const = 0;

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 12)

    VIASTDDECL IsOneWayCall(bool *outOneWay) const = 0;

#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 16)
    // Obtain the index of the return value in the out parameters value.
    // Returns kVIA_ObjectNotFound if the function has void return type.
    VIASTDDECL GetReturnValueMemberIndex(size_t *outIndex) const = 0;

#endif
};

class VIAFbDOMethodProvider : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_MethodProvider
    };

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 17)
    // Obtain a value handle containing all default output arguments of this DO member's next automatic response.
    // [OUT} kVIA_OK if defalut values could be retrieved, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetDefaultOutParamsValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level the value is accessed
            VIAFbValue **outDefaultOutParamsValue) const = 0;           // [OUT] The value object, if successful

#endif
};

class VIAFbDOMethodConsumer : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_MethodConsumer
    };

    // Initiate a function call. The call is invoked by calling InvokeCall on the handle obtained.
    // Note: The obtained outCallContext must be released explicitly via inCallContext->Release.
    // [OUT] kVIA_OK if a call handle could be retrieved, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL BeginCall(
            VIAFbCallContext **outCallContext) const = 0;               // [OUT] Context of the call, if successful

    // Trigger function call that was previously initiated, typically after all in arguments are updated.
    // The observer is notified about all state changes for this call.
    // [OUT] kVIA_OK if the call was successfully sent, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle
    // [THREADSAFE] No
    VIASTDDECL InvokeCall(
            VIAFbDOMethodObserver *inObserver,                          // [IN] Pointer to an object implementing the observer template method (optional, can be nullptr)
            VIAFbCallContext *inCallContext) = 0;                       // [IN] Context of a call obtained from BeginCall

    // Trigger function call that was previously initiated, typically after all in arguments are updated.
    // The observer is notified about all state changes for this call.
    // The call is synchronous: this works only for internal methods in the RT part.
    // [OUT] kVIA_OK if the call was successfully sent, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle
    // [THREADSAFE] No
    VIASTDDECL InvokeSynchronousCall(
            VIAFbDOMethodObserver *inObserver,                          // [IN] Pointer to an object implementing the observer template method (optional, can be nullptr)
            VIAFbCallContext *inCallContext) = 0;                       // [IN] Context of a call obtained from BeginCall
};

class VIAFbDOConsumedMethod : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_ConsumedMethod
    };

    // nothing here yet
};

class VIAFbDOProvidedMethod : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_ProvidedMethod
    };

    // nothing here yet
};

class VIAFbDOInternalMethod : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_InternalMethod
    };

    // nothing here yet
};

class VIAFbDOData : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_Data
    };

    // nothing here yet
};

class VIAFbDODataProvider : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_DataProvider
    };

    // nothing here yet
};

class VIAFbDODataConsumer : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_DataConsumer
    };

    // nothing here yet
};

class VIAFbDOConsumedData : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_ConsumedData
    };

    // nothing here yet
};

class VIAFbDOProvidedData : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_ProvidedData
    };

    // nothing here yet
};

class VIAFbDOInternalData : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_InternalData
    };

    // nothing here yet
};

class VIAFbDOValue : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_Value
    };

    // Retrieve the init value of this DO member.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetInitValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member if this is a composite type
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object, if successful

    // Obtain access to this DO member's data type for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] Yes
    VIASTDDECL GetType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Register an observer to be notified when this DO data member is updated, or the updated value changes.
    // The time the value is received depends on the DO member's binding properties (e.g., cyclic, immediately).
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbDOMemberValueObserver *inObserver,                       // [IN] Pointer to an object implementing the observer template method
            VIAFbUpdateMode inUpdateMode,                               // [IN] Decide to observe only changes in value or any value updates
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback

    // Retrieve the value of this DO member that was last received or set.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member if this is a composite type
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object, if successful

    // Retrieve the status object of this DO member that was last received or set.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetStatus(VIAFbTypeMemberHandle inMemberHandle, VIAFbStatus **outValue) const = 0;

    // Retrieve a user-friendly identifying name
    // [RETURN] kVIA_OK if successful, kVIA_BufferToSmall if buffer too small, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] No
    VIASTDDECL GetNameForErrorMessages(char *inBuffer, size_t inBufferSize) const = 0;
};

class VIAFbDOValueProvider : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_ValueProvider
    };

    // Set the value of this DO data member.
    // [OUT] kVIA_OK if the update was successful, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle
    // [THREADSAFE] No
    VIASTDDECL SetValue(
            const VIAFbValue *inValue) = 0;                             // [IN] The value object containing the new value

};

class VIAFbDOValueConsumer : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_ValueConsumer
    };

    // Lets the member receive a new value (through binding)
    // [OUT] kVIA_OK if the update was successful, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle
    // [THREADSAFE] No
    VIASTDDECL ReceiveValue(const VIAFbValue *inValue) = 0;
};


class VIAFbDOEvent : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_Event
    };

    // nothing here yet
};

class VIAFbDOEventProvider : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_EventProvider
    };

    // Triggers the event
    // [RETURN] Always returns kVIA_OK
    VIASTDDECL TriggerEvent() = 0;
};

class VIAFbDOEventConsumer : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_EventConsumer
    };

    // nothing here yet
};

class VIAFbDOConsumedEvent : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_ConsumedEvent
    };

    // nothing here yet
};

class VIAFbDOProvidedEvent : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_ProvidedEvent
    };

    // nothing here yet
};

class VIAFbDOInternalEvent : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_InternalEvent
    };

    // nothing here yet
};

class VIAFbDOField : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_Field
    };

    // Returns the notification member if the field has one
    // [OUT] the notification member or nullptr
    // [RETURN] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed
    // if no notification exists
    // [THREADSAFE] No
    VIASTDDECL GetNotificationMember(VIAFbDOMember **outNotificationMember) = 0;
    // Returns the getter member if the field has one
    // [OUT] the getter member or nullptr
    // [RETURN] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed
    // if no getter exists
    // [THREADSAFE] No
    VIASTDDECL GetGetterMember(VIAFbDOMember **outGetterMember) = 0;
    // Returns the setter member if the field has one
    // [OUT] the setter member or nullptr
    // [RETURN] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed
    // if no setter exists
    // [THREADSAFE] No
    VIASTDDECL GetSetterMember(VIAFbDOMember **outSetterMember) = 0;
};

class VIAFbDOFieldProvider : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_FieldProvider
    };

    // nothing here yet
};

class VIAFbDOFieldConsumer : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_FieldConsumer
    };

    // nothing here yet
};

class VIAFbDOConsumedField : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_ConsumedField
    };

    // nothing here yet
};

class VIAFbDOProvidedField : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_ProvidedField
    };

    // nothing here yet
};

class VIAFbDOInternalField : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_InternalField
    };

    // nothing here yet
};

// provided by all provided and consumed members
class VIAFbDONetworkAttached : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_NetworkAttached
    };

    // Query whether the member is connected to its network
    // [OUT] connection state
    // [RETURN] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL IsConnected(bool *outIsConnected) = 0;

    // Returns the ID of the network to which the member is attached.
    // You can get the network (if necessary) from the FB service.
    // [OUT] network ID
    // [RETURN] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetNetworkID(uint64 *outID) = 0;
};

// provided by consumed data / event members if the communication pattern is publish/subscribe
class VIAFbDOSubscribable : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_Subscriber
    };

    // Return whether the member is subscribable (communication pattern support)
    // If the member is not subscribable, all other functions of the interface return kVIA_Failed
    VIABOOLDECL IsSubscribable() const = 0;

    // Subscribe the member
    // [RETURN] kVIA_OK if successful, kVIA_Failed otherwise
    // [THREADSAFE] No
    VIASTDDECL Subscribe() = 0;

    // Unubscribe the member
    // [RETURN] kVIA_OK if successful, kVIA_Failed otherwise
    // [THREADSAFE] No
    VIASTDDECL Unsubscribe() = 0;

    // VE for the subscription state
    // [RETURN] kVIA_OK if successful, kVIA_Failed otherwise
    // [THREADSAFE] No
    VIASTDDECL GetSubscriptionState(VIAFbValue **outValue) = 0;
};

// provided by provided data / event members if the communication pattern is publish/subscribe with announce
class VIAFbDOAnnounceable : public VIAFbDOMemberInterface {
public:
    enum MemberInterfaceType {
        InterfaceType = eFbDOMemberIF_Announcer
    };

    // Return whether the member is announceable (communication pattern support)
    // If the member is not announceable, all other functions of the interface return kVIA_Failed
    VIABOOLDECL IsAnnounceable() const = 0;

    // Announce the member
    // [RETURN] kVIA_OK if successful, kVIA_Failed otherwise
    // [THREADSAFE] No
    VIASTDDECL Announce() = 0;

    // Unannounce the member
    // [RETURN] kVIA_OK if successful, kVIA_Failed otherwise
    // [THREADSAFE] No
    VIASTDDECL Unannounce() = 0;

    // VE for the announcement state
    // [RETURN] kVIA_OK if successful, kVIA_Failed otherwise
    // [THREADSAFE] No
    VIASTDDECL GetAnnouncementState(VIAFbValue **outValue) = 0;
};

class VIAFbDONetwork {
public:
    // Release the instance, must be called for any instance obtained from the VIA service to free resources after use.
    // [RETURN] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() const = 0;

    // Returns the name of the network
    // Returns kVIA_OK or kVIA_BufferToSmall or kVIA_ParameterInvalid (if given buffer pointer is nullptr)
    // [OUT] buffer receiving the name
    // [OUT] size of the buffer
    // [THREADSAFE] No
    VIASTDDECL GetName(char *buffer, size_t bufferSize) const = 0;

    // Returns the path of the network (namespaces + name)
    // Returns kVIA_OK or kVIA_BufferToSmall or kVIA_ParameterInvalid (if given buffer pointer is nullptr)
    // [OUT] buffer receiving the path
    // [OUT] size of the buffer
    // [THREADSAFE] No
    VIASTDDECL GetPath(char *buffer, size_t bufferSize) const = 0;

    // Returns the ID of the network
    // Returns kVIA_OK or kVIA_ParameterInvalid (if given pointer is nullptr)
    // [OUT] Pointer receiving the ID.
    // [THREADSAFE] No
    VIASTDDECL GetID(uint64 *outID) const = 0;

    // Access to attributes.
    // Returns kVIA_OK or kVIA_ParameterInvalid (if given pointer is nullptr)
    // [OUT] Pointer receiving the attribute access object. The object must be
    // deleted by calling Release on it.
    // [THREADSAFE] No
    VIASTDDECL GetAttributeAccess(VIAFbAttributeAccess **outAttributeAccess) = 0;
};

class VIAFbDODiscovery {
public:
    // Release the DO's member instance, must be called for any instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // TODO
    //...

};

#endif

// -------------------------------------------------------------------
// VIA FunctionBus: Service CO access

class VIAFbServiceCO {
public:
    // Release the service instance, must be called for any service instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Adds a consumer side dynamic FEP with given name and state to the service.
    // [OUT] kVIA_OK if successfully added consumer, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL AddConsumerByName(
            const char *inName,                                         // [IN] Name of the new service consumer
            VIAFbFEPState inState,                                      // [IN] Whether the new consumer is simulated or real
            VIAFbServiceConsumer **outConsumer) = 0;                    // [OUT] Service consumer handle if successful

    // Adds a consumer side dynamic FEP with given address to the service.
    // [OUT] kVIA_OK if successfully added consumer, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL AddConsumerByAddress(
            const VIAFbAddressHandle *inAddress,                        // [IN] Address to associate with the new service consumer
            VIAFbServiceConsumer **outConsumer) = 0;                    // [OUT] Service consumer handle if successful

    // Removes a consumer side dynamic FEP from the service.
    // [OUT] kVIA_OK if successfully removed, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RemoveConsumer(
            VIAFbServiceConsumer *inConsumer) = 0;                      // [IN] Service consumer handle to be removed.

    // Retrieves a service consumer identified by its binding ID.
    // [OUT] kVIA_OK if consumer was found, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetConsumer(
            VIAFbBindingId inBindingId,                                 // [IN] Binding specific identifier of the consumer
            VIAFbServiceConsumer **outConsumer) = 0;                    // [OUT] Service consumer handle if successful

    // Retrieves a consumer side service port identified by consumer and provider binding IDs.
    // [OUT] kVIA_OK if port was found, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetConsumedService(
            VIAFbBindingId inConsumerId,                                // [IN] Binding specific identifier of the consumer
            VIAFbBindingId inProviderId,                                // [IN] Binding specific identifier of the provider
            VIAFbConsumedService **outPort) = 0;                        // [OUT] Consumer side service port handle if successful

    // Adds a provider side dynamic FEP with given name and state to the service.
    // [OUT] kVIA_OK if successfully added provider, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL AddProviderByName(
            const char *inName,                                         // [IN] Name of the new service provider
            VIAFbFEPState inState,                                      // [IN] Whether the new provider is simulated or real
            VIAFbServiceProvider **outProvider) = 0;                    // [OUT] Service provider handle if successful

    // Adds a provider side dynamic FEP with given address to the service.
    // [OUT] kVIA_OK if successfully added provider, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL AddProviderByAddress(
            const VIAFbAddressHandle *inAddress,                        // [IN] Address to associate with the new service provider
            VIAFbServiceProvider **outProvider) = 0;                    // [OUT] Service provider handle if successful

    // Removes a provider side dynamic FEP from the service.
    // [OUT] kVIA_OK if successfully removed, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RemoveProvider(
            VIAFbServiceProvider *inProvider) = 0;                      // [IN] Service provider handle to be removed.

    // Retrieves a service provider identified by its binding ID.
    // [OUT] kVIA_OK if provider was found, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetProvider(
            VIAFbBindingId inBindingId,                                 // [IN] Binding specific identifier of the provider
            VIAFbServiceProvider **outProvider) = 0;                    // [OUT] Service provider handle if successful

    // Retrieves a provider side service port identified by consumer and provider binding IDs.
    // [OUT] kVIA_OK if port was found, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetProvidedService(
            VIAFbBindingId inConsumerId,                                // [IN] Binding specific identifier of the consumer
            VIAFbBindingId inProviderId,                                // [IN] Binding specific identifier of the provider
            VIAFbProvidedService **outPort) = 0;                        // [OUT] Provider side service port handle if successful

    // Register an observer for service discovery events.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbServiceDiscoveryObserver *inObserver,                  // [IN] Pointer to an object implementing the observer template methods
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered service discovery observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered service discovery callback
};

// -------------------------------------------------------------------
// VIA FunctionBus: Service Consumer/Provider access

class VIAFbServiceConsumer {
public:
    // Release the service consumer instance, must be called for any service consumer instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Provides the consumer's binding specific identifier (kVIABindingIdInvalid if not bound to an address)
    // [OUT] kVIA_OK if successfully retrieved binding ID, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetBindingId(
            VIAFbBindingId *outBindingId) = 0;                          // [OUT] Binding identifier, if successful

    // Initiates service discovery from this consumer.
    // [OUT] kVIA_OK if successfully triggered service discovery, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL DiscoverProviders() = 0;

    // Sets the address of the service consumer itself, or (in the consumer model) sets the address of a known provider.
    // [OUT] kVIA_OK if address successfully set, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL SetAddress(
            const VIAFbAddressHandle *inAddress,                        // [IN] Address to store for this consumer
            VIAFbServiceProvider *inProvider) = 0;                      // [IN] Optional provider which connects to given address

    // Register an observer for service consumer events.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbServiceConsumerObserver *inObserver,                   // [IN] Pointer to an object implementing the observer template methods
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered service consumer observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered service consumer callback

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

class VIAFbServiceProvider {
public:
    // Release the service provider instance, must be called for any service provider instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Provides the provider's binding specific identifier (kVIABindingIdInvalid if not bound to an address)
    // [OUT] kVIA_OK if successfully retrieved binding ID, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetBindingId(
            VIAFbBindingId *outBindingId) = 0;                          // [OUT] Binding identifier, if successful

    // Sets the address of the service provider itself, or (in the provider model) sets the address of a known consumer.
    // [OUT] kVIA_OK if address successfully set, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL SetAddress(
            const VIAFbAddressHandle *inAddress,                        // [IN] Address to store for this provider
            VIAFbServiceConsumer *inConsumer) = 0;                      // [IN] Optional consumer which connects to given address

    // Makes the high level server model announce the service and accept connections
    // [OUT] kVIA_OK if service is successfully provided, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL ProvideService() = 0;

    // Stops the high level server model, making the service unavailable
    // [OUT] kVIA_OK if service is successfully stopped, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL ReleaseService() = 0;

    // Provides whether the high level server model is currently providing the service
    // [OUT] kVIA_OK if service state is successfully retrieved, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL IsServiceProvided(
            bool *outFlag) = 0;                                         // [OUT] Indicates whether the service is currently available

    // Triggers low level service announcement messages
    // [OUT] kVIA_OK if service is successfully stopped, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL AnnounceProvider() = 0;

    // Stops low level service announcement messages
    // [OUT] kVIA_OK if service is successfully stopped, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnannounceProvider() = 0;

    // Retrieve the service state of the service provider.
    // [OUT] kVIA_OK if service state could be determined, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetServiceState(
            VIAFbServiceState *outState) = 0;                           // [OUT] The service state, if successful

    // Register an observer for service provider events.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbServiceProviderObserver *inObserver,                   // [IN] Pointer to an object implementing the observer template methods
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered service provider observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered service provider callback

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

// -------------------------------------------------------------------
// VIA FunctionBus: Service Port access

class VIAFbConsumedService {
public:
    // Release the service port instance, must be called for any service port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Makes the high level client/server model start requesting the service and connecting to the provider
    // [OUT] kVIA_OK if service is successfully stopped, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RequestService() = 0;

    // Stops requesting the service.
    // [OUT] kVIA_OK if service is successfully stopped, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL ReleaseService() = 0;

    // Returns the current client/server model state.
    // [OUT] kVIA_OK if service is successfully stopped, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL IsServiceRequested(
            bool *outFlag) = 0;                                         // [OUT] Indicates whether the service is currently being requested

    // Establishes a connection from the consumer side service port.
    // [OUT] kVIA_OK if connection is successfully initiated, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL ConnectAsync(
            VIAFbConsumedServiceConnectionHandler *inHandler) = 0;      // [IN] Pointer to an object implementing the handler template methods

    // Releases a connection from the consumer side service port.
    // [OUT] kVIA_OK if connection is successfully released, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL Disconnect() = 0;

    // Retrieve the connection state of the consumer side service port.
    // [OUT] kVIA_OK if connection state could be determined, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetConnectionState(
            VIAFbConsumerConnectionState *outState) = 0;                // [OUT] The connection state, if successful

    // Provides the consumer's binding specific identifier (kVIABindingIdInvalid if not bound to an address)
    // [OUT] kVIA_OK if successfully retrieved binding ID, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetConsumerId(
            VIAFbBindingId *outBindingId) = 0;                          // [OUT] Binding identifier, if successful

    // Provides the provider's binding specific identifier (kVIABindingIdInvalid if not bound to an address)
    // [OUT] kVIA_OK if successfully retrieved binding ID, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetProviderId(
            VIAFbBindingId *outBindingId) = 0;                          // [OUT] Binding identifier, if successful

    // Obtain access to this port generically for reading and writing values.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] Yes
    VIASTDDECL GetValuePort(
            VIAFbValuePort **outPort) const = 0;                        // [OUT] Port handle, if successful

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

class VIAFbProvidedService {
public:
    // Release the service port instance, must be called for any service port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Register an observer for simulating server behavior. The callback is invoked for any function
    // being called on server side and shall modify output values of the function.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterSimulator(
            VIAFbFunctionServerObserver *inSimulator,                   // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered simulator.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterSimulator(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered simulator callback

    // Establishes a connection from the provider side service port.
    // [OUT] kVIA_OK if connection is successfully initiated, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL ConnectAsync(
            VIAFbProvidedServiceConnectionHandler *inHandler) = 0;     // [IN] Pointer to an object implementing the handler template methods

    // Releases a connection from the provider side service port.
    // [OUT] kVIA_OK if connection is successfully released, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL Disconnect() = 0;

    // Responds to a service request at this provider side service port.
    // [OUT] kVIA_OK if successfully announced the service, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL AnnounceProvider() = 0;

    // Retrieve the connection state of the provider side service port.
    // [OUT] kVIA_OK if connection state could be determined, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetConnectionState(
            VIAFbProviderConnectionState *outState) = 0;               // [OUT] The value object, if successful

    // Provides the consumer's binding specific identifier (kVIABindingIdInvalid if not bound to an address)
    // [OUT] kVIA_OK if successfully retrieved binding ID, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetConsumerId(
            VIAFbBindingId *outBindingId) = 0;                          // [OUT] Binding identifier, if successful

    // Provides the provider's binding specific identifier (kVIABindingIdInvalid if not bound to an address)
    // [OUT] kVIA_OK if successfully retrieved binding ID, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetProviderId(
            VIAFbBindingId *outBindingId) = 0;                          // [OUT] Binding identifier, if successful

    // Obtain access to this port generically for reading and writing values.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter
    // [THREADSAFE] Yes
    VIASTDDECL GetValuePort(
            VIAFbValuePort **outPort) const = 0;                        // [OUT] Port handle, if successful

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

// -------------------------------------------------------------------
// VIA FunctionBus: Function client/server ports

class VIAFbFunctionClientPort {
public:
    // Release the port instance, must be called for any port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Obtain access to the FunctionBus data type of function's input argument for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] No
    VIASTDDECL GetInParamsType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Obtain access to the FunctionBus data type of function's output argument for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] No
    VIASTDDECL GetOutParamsType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Initiate a function call. The call is invoked by calling InvokeCall on the handle obtained.
    // Note: The obtained outCallContext must be released explicitly via inCallContext->Release.
    // [OUT] kVIA_OK if a call handle could be retrieved, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL BeginCall(
            VIAFbCallContext **outCallContext) const = 0;               // [OUT] Context of the call, if successful

    // Trigger function call that was previously initiated, typically after all in arguments are updated.
    // The observer is notified about all state changes for this call.
    // [OUT] kVIA_OK if the call was successfully sent, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle
    // [THREADSAFE] No
    VIASTDDECL InvokeCall(
            VIAFbFunctionClientObserver *inObserver,                    // [IN] Pointer to an object implementing the observer template method (optional, can be nullptr)
            VIAFbCallContext *inCallContext) = 0;                       // [IN] Context of a call obtained from BeginCall

    // Register an observer to be notified when calls handled by this port change their state.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbFunctionClientObserver *inObserver,                    // [IN] Pointer to an object implementing the observer template method
            VIAFbFunctionCallState inStateToObserve,                    // [IN] The observer is triggered for any call reaching this state
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inCallbackHandle) = 0;            // [IN] Handle from a previously registered observer callback

    // Obtain a value handle containing all input arguments of this port's most recent call.
    // [OUT] kVIA_OK if parameters could be retrieved, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetLatestCall(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level the value is accessed
            VIAFbValue **outInParamsValue) const = 0;                   // [OUT] The value object, if successful

    // Obtain a value handle containing all output arguments of this port's most recently received response.
    // [OUT] kVIA_OK if parameters could be retrieved, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetLatestReturn(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level the value is accessed
            VIAFbValue **outOutParamsValue) const = 0;                  // [OUT] The value object, if successful

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

class VIAFbFunctionServerPort {
public:
    // Release the port instance, must be called for any port instance obtained from the VIA service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Obtain access to the FunctionBus data type of function's input argument for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] No
    VIASTDDECL GetInParamsType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Obtain access to the FunctionBus data type of function's output argument for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] No
    VIASTDDECL GetOutParamsType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Obtain a value handle containing all default output arguments of this port.
    // The first members is a struct with default values for all in/inout arguments (in the same order as the struct obtained from GetOutParams).
    // Inout arguments are accessible as a struct named by the argument. The struct can be used to configure the VSIM for automatic replies:
    // * struct member named "Value" of argument type: value to reply
    // * struct member named "Behavior" with values "UseDefaultValue" = 0, "UseInValue" = 1
    // Additional members of the VSIM functionality are appended:
    // * A special member named "ReturnValue" refers to the return value, it is only defined if the signature's return type is not void.
    // * A special member named "ServerSimulatorMode" with values "Auto" = 0, "OffOrManual" = 1, "Discard" = 2
    // * A special member named "ReturnDelay" with the return to reply time in nano seconds
    // [OUT] kVIA_OK if a call handle could be retrieved, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetDefaultOutParams(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbValue **outDefaultOutParamsValue) const = 0;           // [OUT] The value object, if successful

    // Update the default output arguments for this port.
    // [OUT] kVIA_OK if value successfully set, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL SetDefaultOutParams(
            const VIAFbValue *inDefaultOutParamsValue) const = 0;       // [IN] The value object

    // Register an observer to be notified when calls handled by this port change their state.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbFunctionServerObserver *inObserver,                    // [IN] Pointer to an object implementing the observer template method
            VIAFbFunctionCallState inStateToObserve,                    // [IN] The observer is triggered for any call reaching this state
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inCallbackHandle) = 0;            // [IN] Handle from a previously registered observer callback

    // Obtain a value handle containing all input arguments of this port's most recently received call request.
    // [OUT] kVIA_OK if parameters could be retrieved, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetLatestCall(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level the value is accessed
            VIAFbValue **outInParamsValue) const = 0;                   // [OUT] The value object, if successful

    // Obtain a value handle containing all output arguments of this port's most recent returned call.
    // [OUT] kVIA_OK if parameters could be retrieved, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetLatestReturn(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level the value is accessed
            VIAFbValue **outOutParamsValue) const = 0;                  // [OUT] The value object, if successful

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
#endif
};

// -------------------------------------------------------------------
// VIA FunctionBus: CallContext

// A function call's context. Encapsulates the state and parameters of a function call being processed by client or server side function ports.
class VIAFbCallContext {
public:
    // Release the call context, must be called for any obtained call context to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Obtain a value handle containing all input arguments of this call.
    // * A special member named "RequestId" refers to the internal ID used to identify this call instance, it is ascertained to be globally unique per simulation run.
    // [OUT] kVIA_OK if parameters could be retrieved, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetInParams(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbValue **outInParamsValue) const = 0;                   // [OUT] The value object, if successful

    // Obtain a value handle containing all output arguments of this call.
    // Additional members of the VSIM functionality are appended:
    // * A special member named "RequestId" refers to the internal ID used to identify this call instance, it is ascertained to be globally unique per simulation run.
    // * A special member named "ReturnValue" refers to the return value, it is only defined if the signature's return type is not void.
    // [OUT] kVIA_OK if parameters could be retrieved, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetOutParams(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbValue **outOutParamsValue) const = 0;                  // [OUT] The value object, if successful

    // Update the in parameters value for a particular call.
    // [OUT] kVIA_OK if value successfully updated, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL SetInParams(
            const VIAFbValue *inInParamsValue) = 0;                     // [IN] The value object

    // Update the out parameters value for a particular call.
    // [OUT] kVIA_OK if value successfully updated, kVIA_ParameterInvalid/kVIA_ObjectInvalid in case of a bad parameter or handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL SetOutParams(
            const VIAFbValue *inOutParamsValue) = 0;                    // [IN] The value object

    // Update the time to reply in a particular call context.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL SetTimeToReply(
            VIATimeDiff inRelativeTimeDeltaToReply) = 0;                // [IN] The new time to reply in ns and relative to the current simulation time

    // Get the simulation time when this call was invoked or received.
    // [OUT] kVIA_OK if call time is available, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetCallTime(
            VIATime *outTime) const = 0;                                // [OUT] The time at which the call was invoked (client side) or received (server side)

    // Get the simulation time when this call was returned or a response received.
    // [OUT] kVIA_OK if return time is available, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetReturnTime(
            VIATime *outTime) const = 0;                                // [OUT] The time at which the call was returned (server side) or a response received (client side)

    // Get the current state of this call.
    // [OUT] kVIA_OK if current state can be determined, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetCallState(
            VIAFbFunctionCallState *outState) const = 0;                // [OUT] The current state of the call

    // Determine whether this call is being processed on client or server side.
    // [OUT] kVIA_OK if side can be determined, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetSide(
            VIAFbFunctionCallSide *outSide) const = 0;                  // [OUT] Whether this call is being processed on client or server side

    // Obtain the path to this call's client port.
    // [OUT] kVIA_OK if path was retrieved, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetClientPort(
            char *outBuffer,                                            // [OUT] Pointer to the string buffer to be filled
            std::size_t inBufferLength) const = 0;                           // [IN] Size in bytes of the string buffer

    // Obtain the path to this call's server port.
    // [OUT] kVIA_OK if path was retrieved, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetServerPort(
            char *outBuffer,                                            // [OUT] Pointer to the string buffer to be filled
            std::size_t inBufferLength) const = 0;                           // [IN] Size in bytes of the string buffer

    // Create a permanent call context from this, which can be used beyond teh lifetime of a callback and must be released explicitly.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL MakePermanent(
            VIAFbCallContext **outPermanentContext) const = 0;          // [OUT] The permanent call context object

    // Obtain the unique request ID of this call context.
    // [OUT] kVIA_OK if request ID is valid, kVIA_ObjectInvalid in case of a bad handle
    // [THREADSAFE] Yes
    VIASTDDECL GetRequestID(
            int64 *outRequestID) const = 0;                         // [OUT] The call context's request ID

    // Set out parameters and return value to defaults.
    // [OUT] kVIA_OK if answer was set to defaults, kVIA_ObjectInvalid in case of a bad handle
    // [THREADSAFE] No
    VIASTDDECL SetDefaultAnswer() const = 0;
};

// -------------------------------------------------------------------
// VIA FunctionBus: Value

// A ValueEntity's value. Encapsulates a VVariant and provides access to underlying ISVDataType which can be used to modify members.
// Note: the value is cached and read access does not imply updating the value from the value entity. Modification of members or the
// whole value will only be committed to the value entity through explicit calls on a port or call context object.
class VIAFbValue {
public:
    // Release the value, must be called for any obtained value to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve type of this particular value. Reflection on type is required to obtain a member handle.
    // [OUT] kVIA_OK if type could be returned, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL GetType(
            VIAFbType **outType) const = 0;                             // [OUT] Type of this value

    // Get the access level of this type (Impl/Phys/Raw). To access values at a different level, another value must be retrieved from the port.
    // [OUT] kVIA_OK if type level could be retrieved, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL GetLevel(
            VIAFbTypeLevel *outLevel) const = 0;                        // [OUT] Type level used when accessing this value

    // -------------------------------------------------------------------
    // Getting/setting value of members and root (the latter when inMemberHandle is nullptr)

    // Set value. Value the member handle is pointing to must be of type signed integer.
    // [OUT] kVIA_OK if value could be set, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL SetSignedInteger(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            int64 inValue) = 0;                                        // [IN] Value to be set

    // Get value. Value the member handle is pointing to must be of type signed integer.
    // [OUT] kVIA_OK if value could be retrieved, kVIA_BufferToSmall if buffer is too small, or kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL GetSignedInteger(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            int64 *outValue) const = 0;                                 // [OUT] Pointer to a variable the value will be written to

    // Set value. Value the member handle is pointing to must be of type unsigned integer.
    // [OUT] kVIA_OK if value could be set, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL SetUnsignedInteger(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            uint64 inValue) = 0;                                       // [IN] Value to be set

    // Get value. Value the member handle is pointing to must be of type unsigned integer.
    // [OUT] kVIA_OK if value could be retrieved, kVIA_BufferToSmall if buffer is too small, or kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL GetUnsignedInteger(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            uint64 *outValue) const = 0;                                // [OUT] Pointer to a variable the value will be written to

    // Set value. Value the member handle is pointing to must be of type float.
    // [OUT] kVIA_OK if value could be set, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL SetFloat(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            double inValue) = 0;                                       // [IN] Value to be set

    // Get value. Value the member handle is pointing to must be of type float.
    // [OUT] kVIA_OK if value could be retrieved, kVIA_BufferToSmall if buffer is too small, or kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL GetFloat(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            double *outValue) const = 0;                                // [OUT] Pointer to a variable the value will be written to

    // Set value to data already in the same format as serialized by the FunctionBus.
    // Value the member handle is pointing to must be of type array or string.
    // This can lead to undefined behavior if type or serialization format do not match.
    // [OUT] kVIA_OK if value could be set, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL SetData(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            const void *inValueBuffer,                                  // [IN] Pointer to a memory buffer of values to be set
            std::size_t inNumBytes) = 0;                                     // [IN] Size of memory buffer in bytes

    // Get data value as serialized by the FunctionBus. Value the member handle is pointing to must be of type array or string.
    // This can lead to undefined behavior if type or serialization format do not match.
    // [OUT] kVIA_OK if value could be retrieved, kVIA_BufferToSmall if buffer is too small, or kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL GetData(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            void *outValueBuffer,                                       // [OUT] Pointer to a memory buffer the values will be written to
            std::size_t inNumBytes) const = 0;                               // [IN] Size of memory buffer in bytes

    // -------------------------------------------------------------------
    // Getting/setting length of array

    // Set array length. This value must be an array type.
    // [OUT] kVIA_OK if array length could be set, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL SetArrayLength(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            std::size_t inLength) = 0;                                      // [IN] Array length to be set for this value

    // Set array length. This value must be an array type.
    // [OUT] kVIA_OK if array length could be determined, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL GetArrayLength(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            std::size_t *outLength) const = 0;                               // [OUT] Array length of this value

    // -------------------------------------------------------------------
    // Check/Set if struct member is valid

    // Set a member of this value as a valid member. Value must be of composite type.
    // [OUT] kVIA_OK if flag could be set, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL SetIsValid(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            bool inFlag) = 0;                                           // [IN] Flag that is true iff this member is valid

    // Determine if a member of this value is a valid. Value must be of composite type.
    // [OUT] kVIA_OK if flag could be determined, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL IsValid(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            bool *outFlag) const = 0;                                   // [OUT] Flag that is true iff this member is valid

    // Get currently valid member of a union. Value must be of union type.
    // [OUT] kVIA_OK if member index could be retrieved, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL GetValidMemberIndex(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            std::size_t *outIndex) const = 0;                                // [OUT] Index of the member that is valid

    // -------------------------------------------------------------------
    // Getting/setting string values (in UTF8 encoding)

    // Set value. Value the member handle is pointing to must be of type string.
    // [OUT] kVIA_OK if value could be set, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL SetString(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            const char *inValue) = 0;                                   // [IN] Value to be set

    // Get value. Value the member handle is pointing to must be of type string.
    // [OUT] kVIA_OK if value could be retrieved, kVIA_BufferToSmall if buffer is too small, or kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL GetString(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            char *outBuffer,                                            // [OUT] Pointer to the string buffer to be filled
            std::size_t inBufferLength) const = 0;                           // [IN] Size in bytes of the string buffer

    // Get access to type of member of complex data type value.
    // [OUT] kVIA_OK if type could be retrieved, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetMemberType(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of the composite value
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level the member type is accessed
            VIAFbType **outMemberType) const = 0;                       // [OUT] The type object, if successful

    // Get access to member of complex data type value.
    // [OUT] kVIA_OK if value could be retrieved, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetMemberValue(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of the composite value
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level the member value is accessed
            VIAFbValue **outMemberValue) const = 0;                     // [OUT] The value object, if successful

    // Get access to status of member of complex data type value.
    // [OUT] kVIA_OK if status could be retrieved, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetMemberStatus(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of the composite value
            VIAFbStatus **outMemberStatus) const = 0;                   // [OUT] The status object, if successful

    // Get the size of a raw data buffer required for use in GetData function.
    // [OUT] kVIA_OK if the size could be determined, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetSize(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of the composite value
            std::size_t *outSize) const = 0;                                 // [OUT] The number of bytes required for raw data access

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Commits any pending value modifications to the associated value entity.
    // [OUT] kVIA_OK if the value was successfully committed, kVIA_Failed otherwise
    // [THREADSAFE] No
    VIASTDDECL Commit() = 0;

#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 11)
    // Detaches from the associated value entity, value updates will no longer be received.
    // [OUT] kVIA_OK if the value is successfully detached, kVIA_Failed otherwise
    // [THREADSAFE] No
    VIASTDDECL Detach() = 0;

#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 12)
    // Manually updates the value from the associated value entity
    // [OUT] kVIA_OK
    // [THREADSAFE] No
    VIASTDDECL UpdateValueFromVE() = 0;

#endif
};

// -------------------------------------------------------------------
// VIA FunctionBus: Status

// A ValueEntity's status. Provides access to timestamps of value entity updates and changes.
// Note: as opposed to VIAFbValue the status always operates live on the underlying value entity.
class VIAFbStatus {
public:
    // Release the status, must be called for any obtained status to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Retrieve the value of the associated value entity.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetValue(
            VIAFbTypeLevel inLevel,                                     // [IN] Decide on which type level this value is accessed
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member if this is a composite type
            VIAFbValue **outValue) const = 0;                           // [OUT] The value object, if successful

    // Obtain access to the value entities data type for reflection purposes.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a nullptr
    // [THREADSAFE] No
    VIASTDDECL GetType(
            VIAFbTypeLevel inLevel,                                     // [IN] The type level of the requested type
            VIAFbType **outType) const = 0;                             // [OUT] Type handle, if successful

    // Get the current value state, i.e. whether the value entity was measured or is offline
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL GetValueState(
            VIAFbValueState *outState) const = 0;                       // [OUT] Current state of the value entity

    // Retrieve the absolute time in nanoseconds when this value entity was last updated.
    // [OUT] kVIA_OK if the query was successful, kVIA_Failed in case update never happened
    // [THREADSAFE] No
    VIASTDDECL GetLastUpdateTimestamp(
            VIATime *outTime) const = 0;                                // [OUT] Time of last update, if update occurred

    // Retrieve the absolute time in nanoseconds when this value entity was last changed.
    // [OUT] kVIA_OK if the query was successful, kVIA_Failed in case change never happened
    // [THREADSAFE] No
    VIASTDDECL GetLastChangeTimestamp(
            VIATime *outTime) const = 0;                                // [OUT] Time of last change, if change occurred

    // Register an observer to be notified when this value entity is updated or changed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbValueObserver *inObserver,                             // [IN] Pointer to an object implementing the observer template method
            VIAFbUpdateMode inUpdateMode,                               // [IN] Decide to observe only changes in value or any value updates
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inCallbackHandle) = 0;            // [IN] Handle from a previously registered observer callback

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 7)
    // Reset the status to a state similar to the one at measurement start:
    // Sets the value to the initial value (or zero); sets the value state to Offline; resets change / update counters and flags.
    // The new value is propagated to the CANoe clients, but the binding block doesn't transmit it.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL ResetStatus() = 0;

#endif
};

// -------------------------------------------------------------------
// VIA FunctionBus: Type

// FunctionBus type for reflection and value access
class VIAFbType {
public:
    // Release the type, must be called for any obtained type to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // -------------------------------------------------------------------
    // General type information

    // Retrieve the name of this type
    // [OUT] kVIA_OK if successful, kVIA_ParameterInvalid if buffer too small
    // [THREADSAFE] Yes
    VIASTDDECL GetName(
            char *outBuffer,                                            // [OUT] Pointer to the string buffer to be filled
            std::size_t inBufferLength) const = 0;                           // [IN] Size in bytes of the string buffer

    // Retrieve the name of this type including the full FunctionBus path.
    // [OUT] kVIA_OK if successful, kVIA_ParameterInvalid if buffer too small
    // [THREADSAFE] Yes
    VIASTDDECL GetFullName(
            char *outBuffer,                                            // [OUT] Pointer to the string buffer to be filled
            std::size_t inBufferLength) const = 0;                           // [IN] Size in bytes of the string buffer

    // Determine this type's type tag.
    // [OUT] kVIA_OK if type tag could be determined, kVIA_Failed in case of error
    // [THREADSAFE] Yes
    VIASTDDECL GetTypeTag(
            VIAFbTypeTag *outTypeTag) const = 0;                        // [OUT] Type tag

    // -------------------------------------------------------------------
    // Simple types: Integer, Float, String

    // Retrieve the number of bits reserved for this type.
    // [OUT] kVIA_OK if successful, kVIA_Failed in case of error
    // [THREADSAFE] Yes
    VIASTDDECL GetBitCount(
            std::size_t *outCount) const = 0;                                // [OUT] Number of bits

    // Determine for an integer type if it is signed or unsigned.
    // [OUT] kVIA_OK if successful, kVIA_Failed if this type is not an integer type
    // [THREADSAFE] Yes
    VIASTDDECL IsSigned(
            bool *outFlag) const = 0;                                   // [OUT] True iff this type is signed

    // Determine the length of a string type.
    // This is equivalent to GetArrayMaxLength, because a string is internally defined as an array of uint8.
    // [OUT] kVIA_OK if successful, kVIA_Failed if this type is not a string type
    // [THREADSAFE] Yes
    VIASTDDECL GetStringMaxLength(
            std::size_t *outLength) const = 0;                               // [OUT] Number of 8 bit characters this string can hold

    // Determine the string encoding as Microsoft code page identifier used for display, e.g. 65001 for UTF-8.
    // [OUT] kVIA_OK if encoding could be determined, kVIA_Failed in case of error
    // [THREADSAFE] Yes
    VIASTDDECL GetStringEncoding(
            VIAStringEncoding *outStringEncoding) const = 0;            // [OUT] Encoding enum value

    // -------------------------------------------------------------------
    // Array type

    // Retrieve type of the members of this array type.
    // [OUT] kVIA_OK if type could be returned, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL GetArrayBaseType(
            VIAFbType **outType) const = 0;                             // [OUT] Type of this particular member

    // Retrieve maximum number of members specified for this array type.
    // [OUT] kVIA_OK if number could be determined, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL GetArrayMaxLength(
            std::size_t *outLength) const = 0;                               // [OUT] Maximum number of members this array type can hold

    // Retrieve minimum number of members specified for this array type.
    // [OUT] kVIA_OK if number could be determined, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL GetArrayMinLength(
            std::size_t *outLength) const = 0;                               // [OUT] Minimum number of members this array type must hold

    // Determine if values of this array type have minimum and maximum length set to an equal value.
    // [OUT] kVIA_OK if flag could be determined, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL HasArrayFixedLength(
            bool *outFlag) const = 0;                                   // [OUT] Flag that is true iff arrays of this type are of static size

    // -------------------------------------------------------------------
    // Composite types: Struct, Union

    // Get member handle of this type (relative position within the value entity).
    // [OUT] kVIA_OK
    // [THREADSAFE] Yes
    VIASTDDECL GetMemberHandle(
            VIAFbTypeMemberHandle *outMemberHandle) const = 0;          // [OUT] Member handle of this type

    // Obtain parent member of a previously obtained member handle. This type must be a composite type.
    // [OUT] kVIA_OK if member handle could be determined, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL GetParentMember(
            VIAFbTypeMemberHandle *inoutMemberHandle) const = 0;        // [INOUT] Parent member of ingoing member (can be set to kVIAFbTypeMemberHandleWhole for root)

    // Obtain a member handle relative to a previously obtained member handle, or relative to the root of this composite type.
    // [OUT] kVIA_OK if member handle could be determined, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL GetMember(
            const char *inRelativePath,                                 // [IN] Relative FunctionBus path
            VIAFbTypeMemberHandle *inoutMemberHandle) const = 0;        // [INOUT] Member relative to ingoing member (can be set to kVIAFbTypeMemberHandleWhole for root)

    // Obtain a member handle relative to a previously obtained member handle, or relative to the root of this composite type.
    // [OUT] kVIA_OK if member handle could be determined, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL GetMemberByIndex(
            std::size_t inIndex,                                             // [IN] Index of n-th sub member of the member specified by the given member handle
            VIAFbTypeMemberHandle *inoutMemberHandle) const = 0;        // [INOUT] Member relative to ingoing member (set to kVIAFbTypeMemberHandleWhole for root)

    // Retrieve type of a member of this composite type.
    // [OUT] kVIA_OK if type could be returned, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL GetMemberType(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            VIAFbType **outType) const = 0;                             // [OUT] Type of this particular member

    // Get number of directly specified members of this composite type.
    // [OUT] kVIA_OK if number could be determined, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL GetMemberCount(
            std::size_t *outCount) const = 0;                                // [OUT] Number of directly contained members

    // -------------------------------------------------------------------
    // Composite types: Memory layout applied on raw and implementation type level

    // Determine if a member of this composite type is an optional member. Type must not have a fixed binary layout.
    // [OUT] kVIA_OK if layout could be determined, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL HasFixedBinaryLayout(
            bool *outFlag) const = 0;                                   // [OUT] Flag that is true iff layout is fixed

    // Determine padding of a member of this composite type relative to the preceding member. Binary layout must be fixed.
    // [OUT] kVIA_OK if position could be determined, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL GetMemberRelativeBitOffset(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            std::size_t *outOffset) const = 0;                               // [OUT] Position relative to previous member

    // Determine position of a member of this composite type relative to this composite type. Binary layout must be fixed.
    // [OUT] kVIA_OK if position could be determined, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL GetMemberAbsoluteBitOffset(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            std::size_t *outOffset) const = 0;                               // [OUT] Position relative to this composite type

    // For composite types with fixed binary layout: Determine whether a member's byte order is Big-Endian instead of Little-Endian
    // [OUT] kVIA_OK if byte order could be determined, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL GetMemberByteOrder(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            VIAByteOrder *outByteOrder) const = 0;                      // [OUT] Byte order of the member

    // Determine if a member of this composite type is an optional member.
    // [OUT] kVIA_OK if flag could be determined, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL IsMemberOptional(
            VIAFbTypeMemberHandle inMemberHandle,                       // [IN] Handle to a member of this composite type
            bool *outFlag) const = 0;                                   // [OUT] Flag that is true iff this member is optional

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 10)
    // For arrays: determine if there is a maximum or if the array is unbounded.
    // [OUT] kVIA_OK if flag could be determined, else kVIA_Failed
    // [THREADSAFE] Yes
    VIASTDDECL IsArrayUnbounded(
            bool *outFlag) const = 0;                                   // [OUT] Flag that is true iff this array is unbounded
#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 14)
    // Obtain the name of a sub-member belonging to a previously obtained member, or to the root of this composite type.
    // [OUT] kVIA_OK if name of sub-member could be determined, kVIA_BufferToSmall if buffer too small, else kVIA_Failed
    // [THREADSAFE] No
    VIASTDDECL GetSubMemberName(
            VIAFbTypeMemberHandle inMemberHandle,  // [IN] Handle to a sub-member of this composite type
            char *outSubMemberName,                 //[OUT] Name of the sub-member
            std::size_t inBufferLength) const = 0; //[IN] Size in bytes of the string buffer
#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 15)
    // Returns whether the data type semantically represents a boolean. Note: this can only be true for integer types.
    // [OUT] kVIA_OK if successful, kVIA_Failed in case of error
    // [THREADSAFE] Yes
    VIASTDDECL IsBoolean(bool *outFlag) const = 0; // [OUT] true iff the data type represents a boolean
#endif

};

// -------------------------------------------------------------------
// VIA FunctionBus: Abstract Binding

class VIAFbAbstractBinding {
public:
    // Obtain access to a provider side Event end point with Abstract Binding specific functionality.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if end point does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetEventProvider(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbAbstractEventProvider **outProvider) const = 0;        // [OUT] Event end point handle, if successful

    // Obtain access to a consumer side Event port with Abstract Binding specific functionality.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetConsumedEvent(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbAbstractConsumedEvent **outPort) const = 0;            // [OUT] Event consumer side port handle, if successful

    // Obtain access to a provider side service PDU end point with Abstract Binding specific functionality.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if end point does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetServicePDUProvider(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbAbstractServicePDUProvider **outProvider) const = 0;   // [OUT] Service PDU end point handle, if successful

    // Obtain access to a consumer side service PDU port with Abstract Binding specific functionality.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetConsumedServicePDU(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbAbstractConsumedServicePDU **outPort) const = 0;       // [OUT] Service PDU consumer side port handle, if successful

    // Creates an Abstract Binding specific address.
    // [OUT] kVIA_OK if address is successfully created, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL CreateAddress(
            VIAFbServiceCO *inService,                                  // [IN] The service for which to create an address
            const char *inEndPointName,                                 // [IN] Name of the end point to associate with the address
            VIAFbAddressHandle **outAddress) const = 0;                 // [OUT] The new Abstract Binding specific address

    // Provides the binding identifier associated with an Abstract Binding specific address.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetBindingId(
            VIAFbAddressHandle *inAddress,                              // [IN] The Abstract Binding specific address for which to obtain the identifier
            VIAFbBindingId *outBindingId) const = 0;                    // [OUT] The associated binding identifier

    // Provides the display name associated with an Abstract Binding specific address.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetDisplayName(
            VIAFbAddressHandle *inAddress,                              // [IN] The Abstract Binding specific address for which to obtain the display name
            char *outBuffer,                                            // [OUT] Pointer to the string buffer to be filled
            std::size_t inBufferLength) const = 0;                           // [IN] Size in bytes of the string buffer

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 9)
    // Obtain access to a provider side Field end point with Abstract Binding specific functionality.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if end point does not exist, kVIA_ObjectInvalid if field has no notification
    // [THREADSAFE] Yes
    VIASTDDECL GetFieldProvider(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbAbstractFieldProvider **outProvider) const = 0;        // [OUT] Field end point handle, if successful

    // Obtain access to a consumer side Field port with Abstract Binding specific functionality.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if port does not exist, kVIA_ObjectInvalid if field has no notification
    // [THREADSAFE] Yes
    VIASTDDECL GetConsumedField(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbAbstractConsumedField **outPort) const = 0;            // [OUT] Field consumer side port handle, if successful
#endif
};

class VIAFbAbstractConsumedEvent : public VIAFbConsumedEvent {
public:
    // Release the Event port instance, must be called for any Event port instance obtained from the Abstract Binding service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Initiates high level event request via Abstract Binding.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RequestEvent() = 0;

    // Releases high level event request via Abstract Binding.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL ReleaseEvent() = 0;

    // Provides high level event request state from Abstract Binding.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL IsEventRequested(
            bool *outFlag) = 0;                                         // [OUT] The event's request state

    // Subscribes the consumer side event port low level.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL SubscribeEvent() = 0;

    // Unsubscribes the consumer side event port low level.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnsubscribeEvent() = 0;
};

class VIAFbAbstractEventProvider : public VIAFbEventProvider {
public:
    // Release the Event provider instance, must be called for any Event provider instance obtained from the Abstract Binding service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Register an observer to be notified upon event subscription.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbAbstractEventSubscriptionObserver *inObserver,         // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inCallbackHandle) = 0;            // [IN] Handle from a previously registered observer callback
};

class VIAFbAbstractConsumedServicePDU : public VIAFbConsumedServicePDU {
public:
    // Release the Service PDU port instance, must be called for any Service PDU port instance obtained from the Abstract Binding service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Triggers the high level PDU subscription model to request PDU subscription
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL RequestPDU() = 0;

    // Releases high level PDU subscription
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL ReleasePDU() = 0;

    // Checks whether PDU subscription is currently requested.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL IsPDURequested(
            bool *outFlag) = 0;                                         // [OUT] The current request state of the PDU

    // Initiates low level PDU subscription at this consumer side port.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL SubscribePDU() = 0;

    // Initiates low level PDU desubscription at this consumer side port.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] No
    VIASTDDECL UnsubscribePDU() = 0;
};

class VIAFbAbstractServicePDUProvider : public VIAFbServicePDUProvider {
public:
    // Release the Service PDU provider instance, must be called for any Service PDU provider instance obtained from the Abstract Binding service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Register an observer to be notified when a consumer has subscribed at the PDU provider.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbAbstractPDUSubscriptionObserver *inObserver,           // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback
};

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 9)

class VIAFbAbstractConsumedField : public VIAFbConsumedField {
public:
    // Release the Field port instance, must be called for any Field port instance obtained from the Abstract Binding service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Initiates high level field request via Abstract Binding.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RequestField() = 0;

    // Releases high level field request via Abstract Binding.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL ReleaseField() = 0;

    // Provides high level field request state from Abstract Binding.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL IsFieldRequested(
            bool *outFlag) = 0;                                         // [OUT] The field's request state

    // Subscribes the consumer side field port low level.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL SubscribeField() = 0;

    // Unsubscribes the consumer side field port low level.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnsubscribeField() = 0;
};

class VIAFbAbstractFieldProvider : public VIAFbFieldProvider {
public:
    // Release the Field provider instance, must be called for any Field provider instance obtained from the Abstract Binding service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Register an observer to be notified upon field subscription.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbAbstractFieldSubscriptionObserver *inObserver,         // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inCallbackHandle) = 0;            // [IN] Handle from a previously registered observer callback
};

#endif

// -------------------------------------------------------------------
// VIA FunctionBus: SOME/IP Binding

class VIAFbSomeIPBinding {
public:
    // Obtain access to a provider side event group end point.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if end point does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetEventGroupProvider(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbSomeIPEventGroupProvider **outProvider) const = 0;     // [OUT] Event group end point handle, if successful

    // Obtain access to a consumer side event group port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if end point does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetConsumedEventGroup(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbSomeIPConsumedEventGroup **outPort) const = 0;         // [OUT] Event group port handle, if successful

    // Obtain access to a provider side event group port.
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_ObjectNotFound if end point does not exist
    // [THREADSAFE] Yes
    VIASTDDECL GetProvidedEventGroup(
            const char *inPath,                                         // [IN] FunctionBus model path identifier, pointer to a zero-terminated ASCII string
            VIAFbSomeIPProvidedEventGroup **outPort) const = 0;         // [OUT] Event group port handle, if successful

    // Creates an SOME/IP Binding specific address.
    // [OUT] kVIA_OK if address is successfully created, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL CreateAddress(
            const char *inUDPAddress,                                   // [IN] The IPv4 or IPv6 format address for unreliable communication
            int32 inUDPPort,                                             // [IN] The port number for unreliable communication
            const char *inTCPAddress,                                   // [IN] The IPv4 or IPv6 format address for reliable communication
            int32 inTCPPort,                                             // [IN] The port number for reliable communication
            VIAFbAddressHandle **outAddress) const = 0;                 // [OUT] The new SOME/IP Binding specific address

    // Provides the binding identifier associated with an SOME/IP Binding specific address.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetBindingId(
            VIAFbAddressHandle *inAddress,                              // [IN] The SOME/IP Binding specific address for which to obtain the identifier
            VIAFbBindingId *outBindingId) const = 0;                    // [OUT] The associated binding identifier

    // Provides the display name associated with an SOME/IP Binding specific address.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL GetDisplayName(
            VIAFbAddressHandle *inAddress,                              // [IN] The SOME/IP Binding specific address for which to obtain the display name
            char *outBuffer,                                            // [OUT] Pointer to the string buffer to be filled
            std::size_t inBufferLength) const = 0;                           // [IN] Size in bytes of the string buffer
};

class VIAFbSomeIPEventGroupProvider {
public:
    // Release the SOME/IP Event Group instance, must be called for any SOME/IP Event Group instance obtained from the SomeIP Binding service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Returns the event group's integer identifier
    // [OUT] kVIA_OK if successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetEventGroupId(
            uint32 *outEventGroupId) const = 0;                  // [OUT] The integer identifier of this event group

    // Provides an iterator over the events of this event group.
    // [OUT] kVIA_OK if event iterator was successfully created, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetEvents(
            VIAFbEventIterator<VIAFbEventProvider> **outIterator) = 0;  // [OUT] An iterator over the contained event providers

    // Retrieve an iterator over all service consumers currently subscribed at the event group provider
    // [OUT] kVIA_OK if subscribed consumers could be determined, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetSubscribedConsumers(
            VIAFbServiceConsumerIterator **outIterator) = 0;            // [OUT] An iterator over the currently subscribed consumers

    // Register an observer to be notified upon event group subscription.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterObserver(
            VIAFbSomeIPEventGroupSubscriptionObserver *inObserver,      // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterObserver(
            const VIAFbCallbackHandle inCallbackHandle) = 0;            // [IN] Handle from a previously registered observer callback

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback

    // Provides an iterator over the PDUs of this event group.
    // [OUT] kVIA_OK if PDU iterator was successfully created, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPDUs(
            VIAFbPDUIterator<VIAFbServicePDUProvider> **outIterator) = 0;  // [OUT] An iterator over the contained PDU providers
#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 9)
    // Provides an iterator over the fields of this event group.
    // [OUT] kVIA_OK if field iterator was successfully created, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetFields(
            VIAFbFieldIterator<VIAFbFieldProvider> **outIterator) = 0;  // [OUT] An iterator over the contained field providers
#endif
};

class VIAFbSomeIPConsumedEventGroup {
public:
    // Release the SOME/IP Event Group instance, must be called for any SOME/IP Event Group instance obtained from the SomeIP Binding service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Returns the event group's integer identifier
    // [OUT] kVIA_OK if successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetEventGroupId(
            uint32 *outEventGroupId) const = 0;                  // [OUT] The integer identifier of this event group

    // Provides an iterator over the events of this event group.
    // [OUT] kVIA_OK if event iterator was successfully created, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetEvents(
            VIAFbEventIterator<VIAFbConsumedEvent> **outIterator) = 0;  // [OUT] An iterator over the contained consumed events

    // Initiates high level event group request via SOME/IP Binding.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RequestEventGroup() = 0;

    // Releases high level event group request via SOME/IP Binding.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL ReleaseEventGroup() = 0;

    // Provides high level event group request state from SOME/IP Binding.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL IsEventGroupRequested(
            bool *outFlag) = 0;                                         // [OUT] The event group's request state

    // Subscribes the consumer side event group port low level.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL SubscribeEventGroup() = 0;

    // Unsubscribes the consumer side event group port low level.
    // [OUT] kVIA_OK if the query is successful, kVIA_ParameterInvalid in case of a bad parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnsubscribeEventGroup() = 0;

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback

    // Provides an iterator over the PDUs of this event group.
    // [OUT] kVIA_OK if PDU iterator was successfully created, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPDUs(
            VIAFbPDUIterator<VIAFbConsumedServicePDU> **outIterator) = 0;  // [OUT] An iterator over the contained consumed PDUs
#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 9)
    // Provides an iterator over the fields of this event group.
    // [OUT] kVIA_OK if field iterator was successfully created, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetFields(
            VIAFbFieldIterator<VIAFbConsumedField> **outIterator) = 0;  // [OUT] An iterator over the contained consumed fields
#endif
};

class VIAFbSomeIPProvidedEventGroup {
public:
    // Release the SOME/IP Event Group instance, must be called for any SOME/IP Event Group instance obtained from the SomeIP Binding service to free resources after use.
    // [OUT] kVIA_OK is always returned
    // [THREADSAFE] Yes
    VIASTDDECL Release() = 0;

    // Returns the event group's integer identifier
    // [OUT] kVIA_OK if successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetEventGroupId(
            uint32 *outEventGroupId) const = 0;                  // [OUT] The integer identifier of this event group

    // Provides an iterator over the events of this event group.
    // [OUT] kVIA_OK if event iterator was successfully created, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetEvents(
            VIAFbEventIterator<VIAFbProvidedEvent> **outIterator) = 0;  // [OUT] An iterator over the contained provided events

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 8)
    // Returns the port server's unique identifier for this port
    // [OUT] kVIA_OK if the query was successful, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPortID(
            uint64 *outPortID) const = 0;                   // [OUT] This port's unique identifier, if successful

    // Register an observer to be notified when this port is removed.
    // [OUT] kVIA_OK if registration was successful, kVIA_ParameterInvalid in case of a bad in parameter, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL RegisterPortObserver(
            VIAFbPortObserver *inObserver,                              // [IN] Pointer to an object implementing the observer template method
            VIAFbCallbackHandle *outCallbackHandle) = 0;                // [OUT] Handle to the callback, if successful

    // Unregister a previously registered port observer.
    // [OUT] kVIA_OK if deregistration was successful, kVIA_ParameterInvalid in case of a bad handle, kVIA_Failed in case of an error
    // [THREADSAFE] No
    VIASTDDECL UnregisterPortObserver(
            const VIAFbCallbackHandle inHandle) = 0;                    // [IN] Handle from a previously registered observer callback

    // Provides an iterator over the PDUs of this event group.
    // [OUT] kVIA_OK if PDU iterator was successfully created, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetPDUs(
            VIAFbPDUIterator<VIAFbProvidedServicePDU> **outIterator) = 0;  // [OUT] An iterator over the contained provided PDUs
#endif

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 9)
    // Provides an iterator over the fields of this event group.
    // [OUT] kVIA_OK if field iterator was successfully created, kVIA_ParameterInvalid in case of a bad parameter
    // [THREADSAFE] No
    VIASTDDECL GetFields(
            VIAFbFieldIterator<VIAFbProvidedField> **outIterator) = 0;  // [OUT] An iterator over the contained provided fields
#endif
};

// ============================================================================
#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 11)

// A binding for a single DO member
class VIAFbDOBinding {
public:
    // the member is connected to its virtual network
    VIASTDDECL Connect() = 0;
    // the member is disconnected from its virtual network
    VIASTDDECL Disconnect() = 0;
    // communication pattern publish/subscribe: the consumed member shall be subscribed
    VIASTDDECL Subscribe() = 0;
    // communication pattern publish/subscribe: the provided member shall be announced
    VIASTDDECL Announce() = 0;
    // communication pattern publish/subscribe: the consumed member shall be unsubscribed
    VIASTDDECL UnSubscribe(bool calledOnDisconnect) = 0;
    // communication pattern publish/subscribe: the provided member shall be unannounced
    VIASTDDECL UnAnnounce() = 0;
    // Transmit of data for the member was triggered: the binding shall now transmit the data.
    // The binding must get the data from the relevant VE(s).
    // Note 1: the binding shall not react itself to VE changes, it shall only transmit the data
    // when this method is called.
    // Note 2: it may still depend on the communication pattern whether data is actually transmitted.
    // E.g., the TransmitTriggered() method may still be called even if the pattern is PublishSubscribe
    // with announcement and the member has not yet been announced.
    // Note 3: it is not required that the binding transmits the data immediately on the
    // bus when the method is called. The binding can transmit the data on its own discretion,
    // e.g. put a signal on a PDU and transmit the PDU according to binding configuration.
    // But data shall not be transmitted as long as the method is not called.
    VIASTDDECL TransmitTriggered() = 0;
};


// Interface for DLLs which implement a binding block
class VIAFbDOBindingBlock {
public:
    // the version of the interface which the binding block supports
    // this is the VIAFunctionBusServiceMinorVersion against which it was compiled
    VIASTDDECL GetSupportedInterfaceVersion(int32 *pFBServiceVersion) = 0;

    // name of the binding, also value for the [Binding] attribute to select the binding
    VIASTDDECL GetBindingName(char *buffer, size_t bufferSize) = 0;

    // create a binding for a DO member
    // the binding shall take ownership of the member, i.e. it must call Release on the member
    VIASTDDECL CreateBinding(VIAFbDOMember *pMember, VIAFbDOBinding **outBinding) = 0;
    // remove a single DO member binding. Used if the DO instance is dynamically removed
    VIASTDDECL RemoveBinding(VIAFbDOBinding *pBinding) = 0;
    // remove all bindings
    VIASTDDECL RemoveAllBindings() = 0;

    // Function Bus Data Model is about to be unloaded
    VIASTDDECL OnFBLibraryClosing() = 0;
    // (new) Function Bus Data Model is loaded
    VIASTDDECL OnFBLibraryChanged() = 0;

    // Measurement notifications
    VIASTDDECL OnMeasurementInit() = 0;

    VIASTDDECL OnMeasurementStart() = 0;

    VIASTDDECL OnMeasurementStop() = 0;

    VIASTDDECL OnMeasurementEnd() = 0;

#if defined(VIAFunctionBusServiceMinorVersion) && (VIAFunctionBusServiceMinorVersion >= 13)
    // returns the values for the Binding attribute for which this binding block is used
    // returns kVIA_ObjectNotFound if valueNr is above the number of attribute values for this binding block
    VIASTDDECL GetBindingAttributeValue(size_t valueNr, char *buffer, size_t bufferSize) = 0;

#endif
};

// Create the binding block. This will be called after VIAGetRequiredVersion and VIAGetDesiredVersion.
// VIASetService is not called for binding block DLLs, instead the service is passed as a parameter.
VIACLIENT(VIAFbDOBindingBlock*) VIACreateDOBindingBlock(VIAService *pVIAService);
// Release the binding block.
VIACLIENT(void) VIAReleaseDOBindingBlock(VIAFbDOBindingBlock *pBindingBlock);

#endif

#endif // VIA_FUNCTIONBUS_H