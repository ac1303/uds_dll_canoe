/*-----------------------------------------------------------------------------
Module: VIA (observed)
Interfaces: This is a public interface header.
-------------------------------------------------------------------------------
VIA interfaces for Ethernet busses.
-------------------------------------------------------------------------------
Copyright (c) Vector Informatik GmbH. All rights reserved.
-----------------------------------------------------------------------------*/

#ifndef VIA_ETHERNET_H
#define VIA_ETHERNET_H


#ifndef VIA_H
#include "VIA.h"
#endif

#ifndef VIA_SOCKET_H

#include "VIA_Socket.h"

#endif

class VIDBEthernetNode;

class VIDBApplicationEndpoint;

class VIDBProvidedServiceInstance;

class VIDBServiceInstance;

class VIDBConsumedServiceInstance;

class VIDBProvidedEventGroup;

class VIDBConsumedEventGroup;

class VIDBServiceInterface;

class VIDBMethod;

class VIDBField;

class VIDBEventGroup;

class VIDBEvent;

class VIDBFieldNotifier;

class VIDBDataType;

class VIDBArrayDimension;

class VIDBInputParameter;

class VIDBOutputParameter;

class VIDBPDUTransmission;

class VIDBRemoteApplicationEndpoint;

class VIAPDUService;

class VIAEthernetPort;

namespace NIPB {
    class INetworkModel;

    class IProtocolManager;

    class ISocketManager;

    class ISocketAdapter;

    class IPacket;

    class IApi;
}

// ============================================================================
// Version of the interfaces, which are declared in this header
// See  REVISION HISTORY for a description of the versions
// 04.08.2009   1.3  Jr     Extended VIAEthernetBus to retrieve Ethernet adapter status 
// 25.08.2009   1.4  Jr     Added GetThisPacket
// 28.06.2010   1.5  Jr     Added GetSocketAdapter
// 03.09.2013   1.7  Jmi    Added ConfigureEthernetPacketGenerator, StartEthernetPacketGenerator
// 03.06.2014   1.8  Ste    Added classes VIDBEthernetNode, VIDBApplicationEndpoint, VIDBProvidedServiceInstance,
//                          VIDBServiceInstance, VIDBConsumedServiceInstance, VIDBProvidedEventGroup,
//                          VIDBConsumedEventGroup, VIDBServiceInterface, VIDBMethod, VIDBField,
//                          VIDBEventGroup and the function GetEthernetNode(...)
// 27.10.2014   1.9  sbj    added GetE2EProtectionInfo(..)
// 25.02.2015   1.10 ste    Add classes VIDBEvent and VIDBFieldNotifier. Modified in consequence the event iterator
//                          type in VIDBServiceInterface and VIDBEventGroup. Moreover the VIDBField class receives a
//                          new function: GetFieldNotifier(..). In addition both functions
//                          GetApplicationCycleInMs(..) and GetDebounceTimeDurationInMs(..) have been added to both
//                          VIDBEvent and VIDBFieldNotifier. Add five SetValue2... functions to VIASomeIpMessage,
//                          all "overloading" the previous SetValue... functions with a new out parameter
//                          "valueHasChanged" indicating whether the value has been changed by the set operation.
//                          Add new functions SetValueAsData and SetValueAsData2.
// 09.07.2015   1.12 wan    Added class VIDBDataType in order to fetch information about data types (e.g. of VIDBField).
//                          Furthermore added class VIDBArrayDimension to determine whether the data type is an array
//                          and the classes VIDBInputParameter and VIDBOutputParameter which are used to get information
//                          about the parameters of VIDBMethods and VIDBEvents.
// 26.08.2015   1.13 wan    Extended VIAEthernetBus in order to retrieve the service interfaces that are defined for the
//                          network (which is necessary, as e.g. GM FSA does not define the consumed services in the DB)
// 21.01.2016   1.16 sbj    Added VIDBPDUTransmission, VIDBRemoteApplicationEndPoint and GetPDUService
// 03.03.2016   1.17 dml    Added several End-to-EndProtectionInfo Getter
// 01.09.2016   1.18 dml    Add Event property "TriggeredOnSubsciption" to get field behaviour @AUTOSAR
// 13.02.2016   1.20 dml    Connection socket address information added
// 12.04.2017   1.21 jr     Added GetEthernetPacketCAPLData
// 16.10.2017   1.22 wan    Added ValidateMessage that may be used for SecOC-Validation of SOME/IP-Messages
// 12.12.2017   1.23 sbj    Added priority properties
// 30.04.2018   1.24 jmi    Added Status Callback with hwChannel
// 03.07.2018   1.25 sbj    Added InitFromPDUData to SOME/IP message
// 18.01.2019   1.26 sbj    Allow more than one address per application endpoint and added source type (Fixed, DHCP, ...)
// 18.02.2019   1.27 sbj    Ethernet Ports iterator and IP address/endpoints
// 17.05.2019   1.28 sbj    IsSwitchNetwork property and GetThisFrameEthernetPort
// 23.08.2019   1.29 pit    GetNetworkCycleInMs() 
// 10.09.2020   1.30 pit    IsOptional() 
// ============================================================================

#if !defined ( VIAEthernetMajorVersion )
#define VIAEthernetMajorVersion 1
#endif
#if !defined ( VIAEthernetMinorVersion )
#define VIAEthernetMinorVersion 30
#endif

// ============================================================================
// Ethernet bus interface
// ============================================================================

// 
// Callback object to receive a packet from Ethernet bus
//
class VIAOnEthernetPacket {
public:
    VIASTDDECL OnEthernetPacket(VIATime time, VIAChannel channel, uint8 dir, uint32 packetSize,
                                const uint8 *packetData) = 0;
};


// 
// Callback object to receive status change events from Ethernet bus
//
class VIAOnEthernetStatus {
public:
    typedef enum {
        kStatusUnknown = 0,
        kNotConnected = 1,
        kConnected = 2
    } VStatus;

public:
    // Handler which is called on status changes of the Ethernet adapter
    VIASTDDECL OnEthernetStatus(VIATime time, VIAChannel channel, uint32 status) = 0;
};

//
// Callback object to receive status change events from Ethernet bus with hw channel
//
class VIAOnEthernetStatusWithHwChannel {
public:
    typedef enum {
        kStatusUnknown = 0,
        kNotConnected = 1,
        kConnected = 2
    } VStatus;
public:
    // Handler which is called on status changes of the Ethernet adapter
    VIASTDDECL OnEthernetStatus(VIATime time, VIAChannel channel, VIAHwChannel hwchannel, uint32 status) = 0;
};

// 
// Callback object to receive a Ethernet PDU (FSA, ... ) from Ethernet bus
//
class VIAOnEthernetPdu {
public:
    typedef enum {
        kIPv4 = 0x01,
        kIPv6 = 0x02,
        kTL_UDP = 0x04,
        kTL_TCP = 0x08,
        kPayloadTruncated = 0x10,
    } VFlag;

    typedef enum {
        kPduFsa = 1
    } VPduType;

    struct VPduInfo {
        uint16 structSize;
        uint8 pduType;
        uint8 dir;
        uint16 flag;
        VIAChannel channel;
        VIATime time;
        uint16 sourcePort;
        uint16 destinationPort;
        uint8 sourceAddress[16];      // IPv4 or IPv6 address depends on flag
        uint8 destinationAddress[16]; // IPv4 or IPv6 address depends on flag
        uint32 headerSize;
        const uint8 *headerData;
        uint32 payloadSize;
        const uint8 *payloadData;
    };

public:
    VIASTDDECL OnEthernetPdu(const VPduInfo *pduInfo) = 0;
};

// ============================================================================
// class VIASomeIpMessage
// ============================================================================

class VIASomeIpMessage {
public:
    // ============================================================================
    // Version of this interfaces
    // See  REVISION HISTORY for a description of the versions
    // 30.11.2012   1.0  Amr    Add VIASomeIpMessage
    // ============================================================================

    static const int32 kVersionMajor = 1;
    static const int32 kVersionMinor = 1;

    typedef enum {
        kRequest = 0x00, // REQUEST                A request expecting a response (even void)
        kRequestNoReturn = 0x01, // REQUEST_NO_RETURN      A fire&forget request
        kNotification = 0x02, // NOTIFICATION           A request of a notification/event callback expecting no response
        kRequestAck = 0x40, // REQUEST ACK            Acknowledgment for REQUEST (optional)
        kRequestNoReturnAck = 0x41, // REQUEST_NO_RETURN ACK  Acknowledgment  for  REQUEST_NO_RETURN  (informational)
        kNotificationAck = 0x42, // NOTIFICATION ACK       Acknowledgment for NOTIFICATION (informational)
        kResponse = 0x80, // RESPONSE               The response message
        kError = 0x81, // ERROR                  The response containing an error
        kResponseAck = 0xC0, // RESPONSE ACK           Acknowledgment for RESPONSE (informational)
        kErrorAck = 0xC1, // ERROR ACK              Acknowledgment for ERROR (informational)
    } VMessageType;

    typedef enum {
        kOk = 0x00, //  E_OK  No error occurred
        kNotOk = 0x01, //  E_NOT_OK  An unspecified error occurred
        kUnknownService = 0x02, //  E_UNKNOWN_SERVICE  The requested Service ID is unknown
        kUnknownMethod = 0x03, //  E_UNKNOWN_METHOD  The  requested  Method  ID  is  unknown,  Service  ID  is known
        kNotReady = 0x04, //  E_NOT_READY  Service  ID  and  Method  ID  are  known,  Application  not running
        kNotReachable = 0x05, //  E_NOT_REACHABLE  System  running  the  service  is  not  reachable  (internal error code only)
        kTimeout = 0x06, //  E_TIMEOUT  A timeout occurred (internal error code only)
        kWrongProtocolVersion = 0x07, //  E_WRONG_PROTOCOL_VERSION  Version of SOME/IP protocol not supported
        kWrongInterfaceVersion = 0x08, //  E_WRONG_INTERFACE_VERSION  Interface version mismatch
        kMalFormedMessage = 0x09, //  E_MALFORMED_MESSAGE  Deserialization error (eg length or type incorrect)
        // reserved 0x0a..0x3f
    } VReturnCode;

    typedef enum : int32 {
        kSecOcSecuredInvalid = -1, // SecOC-Information available, but validation failed
        kUnsecured = 0,  // no SecOC-Information available
        kSecOcSecuredValid = 1   // SecOC-Information available and validation successful
    } ValidationState;

    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Initialize the SOME/IP message from a serialized buffer
    /*!
        \param  major                 Major version number
        \param  minor                 Minor version number

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL GetVersion(int32 * /*out*/ major, int32 * /*out*/ minor) const = 0;

    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Initialize the SOME/IP message from a serialized buffer
    /*!
        \param  bufferLength          The buffer's length passed as parameter
        \param  buffer                The buffer containing the message data to deserialize

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL InitFromRawData(uint32 /*in*/ bufferLength, const unsigned char /*in*/ *buffer) = 0;

    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Initialize an empty SOME/IP message
    /*!
        \param  messageId             Service ID and Method ID
        \param  requestId             Client ID and Session ID
        \param  protocolVersion       Protocol Version, must match Fibex
        \param  interfaceVersion      Interface Version, must match Fibex
        \param  messageType           Message Type
        \param  returnCode            Return Code

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL Init(uint32 messageId, uint32 requestId, uint8 protocolVersion, uint8 interfaceVersion,
                    VMessageType messageType, VReturnCode returnCode) = 0;

    /*-------------------------------------------------------------------------------------------------------------------------*/
    //! Serialize a SOME/IP message
    /*!
        \param  bufferLength          In: the length of the buffer passed as parameter / Out: return the number of bytes copied
        \param  buffer                Return the buffer containing the payload data serialized with SOME/IP or NULL

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL Serialize(uint32 /*in/out*/ *bufferLength, unsigned char /*out*/    *buffer) const = 0;

    /*-------------------------------------------------------------------------------------------------------------------------*/
    //! Get SOME/IP message header field
    /*!
        \param  messageId             Service ID and Method ID
        \param  requestId             Client ID and Session ID
        \param  protocolVersion       Protocol Version, must match Fibex
        \param  interfaceVersion      Interface Version, must match Fibex
        \param  messageType           Message Type
        \param  returnCode            Return Code

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL GetMessageId(uint32 /*out*/ *messageId) const = 0;

    VIASTDDECL GetLength(uint32 /*out*/ *length) const = 0;

    VIASTDDECL GetRequestId(uint32 /*out*/ *requestId) const = 0;

    VIASTDDECL GetProtocolVersion(uint8 /*out*/ *protocolVersion) const = 0;

    VIASTDDECL GetInterfaceVersion(uint8 /*out*/ *interfaceVersion) const = 0;

    VIASTDDECL GetMessageType(VMessageType /*out*/ *messageType) const = 0;

    VIASTDDECL GetReturnCode(VReturnCode /*out*/ *returnCode) const = 0;

    /*-------------------------------------------------------------------------------------------------------------------------*/
    //! Set SOME/IP message header field
    /*!
        \param  requestId             Client ID and Session ID
        \param  returnCode            Return Code

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL SetRequestId(uint32 /*in*/ requestId) = 0;

    VIASTDDECL SetReturnCode(VReturnCode /*in*/ returnCode) = 0;

    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Get SOME/IP message payload field
    /*!
        \param  valuePath             The path of the value
        \param  value                 Return the value we asked

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL GetValueAsInt32(const char /*in*/ *valuePath, int32 /*out*/ *value) const = 0;

    VIASTDDECL GetValueAsInt64(const char /*in*/ *valuePath, int64 /*out*/ *value) const = 0;

    VIASTDDECL GetValueAsDouble(const char /*in*/ *valuePath, double /*out*/ *value) const = 0;

    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Get SOME/IP message payload field
    /*!
        \param  valuePath             The path of the value we want to retrieve
        \param  bufferLength          In: the length of the buffer passed as parameter / Out: return the number of bytes copied
        \param  value                 Return the buffer containing the value we asked

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL GetValueAsString(const char /*in*/ *valuePath, uint32 /*in/out*/ *bufferLength,
                                unsigned char /*out*/ *value) const = 0;

    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Set SOME/IP message payload field
    /*!
        \param  valuePath             The path of the value
        \param  value                 The new value

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL SetValueAsInt32(const char /*in*/ *valuePath, int32 /*in*/ value) = 0;

    VIASTDDECL SetValueAsInt64(const char /*in*/ *valuePath, int64 /*in*/ value) = 0;

    VIASTDDECL SetValueAsDouble(const char /*in*/ *valuePath, double /*in*/ value) = 0;

    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Set SOME/IP message payload field
    /*!
        \param  valuePath             The path of the value we want to retrieve
        \param  bufferLength          The length of the buffer passed as parameter
        \param  value                 The buffer containing the new value

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL SetValueAsString(const char /*in*/ *valuePath, uint32 /*in*/ bufferLength,
                                const unsigned char /*in*/ *value) = 0;

    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Delete SOME/IP message payload field
    /*!
        \param  valuePath             The path of the value we want to set

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL DeleteValue(const char /*in*/ *valuePath) = 0;

    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Get SOME/IP message payload
    /*!
        \param  bufferLength          In: the length of the buffer passed as parameter / Out: return the number of bytes copied
        \param  payload               Return the buffer containing the payload

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL GetPayload(uint32 /*in/out*/ *bufferLength, unsigned char /*out*/ *payload) const = 0;

    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Set SOME/IP message payload
    /*!
        \param  bufferLength          The length of the buffer passed as parameter
        \param  payload               The buffer containing the payload

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL SetPayload(uint32 /*in*/ bufferLength, const unsigned char /*in*/ *value) = 0;


#if defined ( VIAMinorVersion) && (VIAMinorVersion >= 53)
    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Get SOME/IP message payload field
    /*!
        \param  valuePath             The path of the value
        \param  value                 Return the value we asked

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL GetValueAsPhys(const char /*in*/ *valuePath, double /*out*/ *value) const = 0;

    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Set SOME/IP message payload field
    /*!
        \param  valuePath             The path of the value
        \param  value                 The new value

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL SetValueAsPhys(const char /*in*/ *valuePath, double /*in*/ value) = 0;

#endif

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 10)
    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Set SOME/IP message payload field
    /*!
        \param  valuePath             The path of the value
        \param  newValue              The new value
        \param  valueHasChanged       TRUE if the new value set was different from the old one.
                                      FALSE if the new value was the same as the old one and the old
                                      value was merely "updated"

        \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL SetValueAsInt32_2(const char /*in*/ *valuePath, int32 /*in*/ newValue,
                                 bool /*in/out*/ *valueHasChanged) = 0;

    VIASTDDECL SetValueAsInt64_2(const char /*in*/ *valuePath, int64 /*in*/ newValue,
                                 bool /*in/out*/ *valueHasChanged) = 0;

    VIASTDDECL SetValueAsDouble_2(const char /*in*/ *valuePath, double /*in*/ newValue,
                                  bool /*in/out*/ *valueHasChanged) = 0;

    VIASTDDECL SetValueAsString_2(const char /*in*/ *valuePath, uint32 /*in*/ bufferLength,
                                  const unsigned char /*in*/ *newValue, bool /*in/out*/ *valueHasChanged) = 0;

    VIASTDDECL SetValueAsPhys_2(const char /*in*/ *valuePath, double /*in*/ newValue,
                                bool /*in/out*/ *valueHasChanged) = 0;

    VIASTDDECL SetValueAsData(const char /*in*/ *valuePath, uint32 /*in*/ bufferLengthInBits,
                              const unsigned char /*in*/ *newValue) = 0;

    VIASTDDECL SetValueAsData_2(const char /*in*/ *valuePath, uint32 /*in*/ bufferLengthInBits,
                                const unsigned char /*in*/ *newValue, bool /*in/out*/ *valueHasChanged) = 0;

#endif


#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 25)
    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Initialize the SOME/IP message from a serialized buffer without AUTOSAR PDU header
    /*!
    \param  headerId              AUTOSAR pdu header id
    \param  bufferLength          The buffer's length passed as parameter
    \param  buffer                The buffer containing the pdu data to deserialize

    \return                       An error code indicating if the function call was successful or not
    */
    VIASTDDECL InitFromPDUData(uint32 headerId, uint32 /*in*/ bufferLength,
                               const unsigned char /*in*/ *buffer) = 0;

    /*-------------------------------------------------------------------------------------------------------------------------*/
    //! Serialize a SOME/IP message to an AUTOSAR PDU buffer
    /*!
    \param  bufferLength          In: the length of the buffer passed as parameter / Out: return the number of bytes copied
    \param  buffer                Return the buffer containing the SOME/IP message without AUTOSAR PDU header or NULL

    \return                       An error code indicating if the function call was successful or not
    */

    VIASTDDECL SerializePDUData(uint32 /*in/out*/ *bufferLength, unsigned char /*out*/    *buffer) const = 0;


    /*-------------------------------------------------------------------------------------------------------------------------*/
    //! Serialize a SOME/IP message to an AUTOSAR PDU buffer
    /*!
    \param  copiedMessage         new SOME/IP message

    \return                       An error code indicating if the function call was successful or not
    */
    VIASTDDECL CreateCopy(VIASomeIpMessage **copiedMessage) const = 0;

#endif

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 30)
    /*--------------------------------------------------------------------------------------------------------------------------*/
    //! Get if member for memberPath is optional or not
    /*!
    \param  memberPath            The path of the member
    \param  isOptional            Return if member is optional

    \return                       An error code indicating if the function call was successful or not
    */
    VIASTDDECL IsOptional(const char /*in*/ *memberPath, bool /*out*/ *isOptional) const = 0;

#endif
};


// 
// Callback object to receive status change events from Ethernet bus
//
class VIAOnEthernetPacketError {
public:
    typedef enum {
        kErrorUnknown = 0,
        kErrorRxDataLengthError = 1,
        kErrorRxInvalidCrc = 2,
        kErrorRxPhyError = 3,
        kErrorRxCollision = 4,
        kErrorTxNoLink = 5,
        kErrorTxBypassEnabled = 6,
        kErrorTxWrongTime = 7
    } VError;

public:
    // Handler which is called on rx error events of the Ethernet adapter
    VIASTDDECL OnEthernetPacketError(VIATime time, VIAChannel channel, uint8 dir, uint32 errorCode, uint32 fcs,
                                     uint32 packetLength, const uint8 *packetData) = 0;
};



// ============================================================================
// class VIAEthernetBus
// ============================================================================

class VIAEthernetBus
        : public VIABus {
public:
    // Release a Request (MessageRequest or ErrorFrameRequest)
    VIASTDDECL ReleaseRequest(VIARequestHandle handle) = 0;

    // Put a message on the Ethernet bus
    VIASTDDECL OutputEthernetPacket(VIAChannel channel, uint32 packetSize, const uint8 *packetData) = 0;

    // Registration for Ethernet packets
    VIASTDDECL CreateAllPacketRequest(VIARequestHandle *handle,   // the created request handle
                                      VIAOnEthernetPacket *sink,  // callback object
                                      VIAChannel channel) = 0;   // channel

    // Get MAC ID of local Ethernet adapter of this bus
    VIASTDDECL GetAdapterMacId(uint32 macIdBufferSize, uint8 *macIdBuffer) = 0;

    // Get IPB network model
    VIASTDDECL GetNetworkModel(NIPB::INetworkModel **networkModel) = 0;

    // Get IPB protocol manager
    VIASTDDECL GetProtocolManager(NIPB::IProtocolManager **protocolManager) = 0;

    // Get IPB socket manager
    VIASTDDECL GetSocketManager(NIPB::ISocketManager **socketManager) = 0;

    // Register for Ethernet adapter status notification
    VIASTDDECL CreateStatusRequest(VIARequestHandle *handle,  // the created request handle
                                   VIAOnEthernetStatus *sink, // callback object
                                   VIAChannel channel) = 0;  // channel

    // Get status of Ethernet adapter
    VIASTDDECL GetAdapterStatus(uint32 *status) = 0;

    // Returns the current received packet within VIAOnEthernetPacket::OnEthernetPacket
    VIASTDDECL GetThisPacket(NIPB::IPacket **packet) = 0;

    // Returns the socket adapter of the node
    VIASTDDECL GetSocketAdapter(void **adapter) = 0;

    // Returns an empty SOME/IP message
    VIASTDDECL CreateSomeIpMessage(VIASomeIpMessage /*out*/ **message) = 0;
    // Deletes a SOME/IP message
    VIASTDDECL ReleaseSomeIpMessage(VIASomeIpMessage /*in*/ *message) = 0;
    // Returns the current received packets frame check sequence within VIAOnEthernetPacket::OnEthernetPacket
    VIASTDDECL GetThisFrameCheckSequence(uint32 *fcs) = 0;

    // Put a message on the Ethernet bus with frame check sequence
    VIASTDDECL OutputEthernetPacketWithFcs(VIAChannel channel, uint32 fcs, uint32 packetSize,
                                           const uint8 *packetData) = 0;

    // Register for Ethernet error packet
    VIASTDDECL CreateEthernetPacketRxErrorRequest(VIARequestHandle *handle,  // the created request handle
                                                  VIAOnEthernetPacketError *sink, // callback object
                                                  VIAChannel channel) = 0;  // channel

    // configure a Ethernet packet generator
    VIASTDDECL ConfigureEthernetPacketGenerator(VIAChannel channel, uint32 packetSize, const uint8 *packetData,
                                                uint32 transmissionRate, uint32 counterType,
                                                uint32 counterPosition) = 0;

    // start or stop a Ethernet packet generator
    VIASTDDECL StartEthernetPacketGenerator(VIAChannel channel, uint32 numberOfFrames) = 0;


#if defined ( VIAMinorVersion) && (VIAMinorVersion >= 52)
    // Retrieves messages by ID.
    VIASTDDECL GetMessage(uint32 ID, VIDBMessageDefinition **message) = 0;
    // You should retrieve messages using the ID. Use the name only if the ID is not available.
    VIASTDDECL GetMessage(const char *name, VIDBMessageDefinition **message) = 0;

    // Gets access to attributes of the network / bus itself:
    // Returns the attribute / the attribute type / the numeric value / the string value
VIDB_DECLARE_ATTRIBUTE_HOLDER

    // Retrieves the definition of an arbitrary node on the bus
    VIASTDDECL GetNodeDefinition(const char *nodeName, VIDBNodeDefinition **nodeDefinition) = 0;
    // Iterates over nodes
    VIDB_DECLARE_ITERATABLE(NodeDefinition)

#endif

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 8)
public:
    VIASTDDECL GetEthernetNode(VIDBEthernetNode **ethernetNode) const = 0;

#endif

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 11)
public:
    // Put a message on the Ethernet bus at a specific point in time
    VIASTDDECL OutputEthernetPacketAtTime(VIAChannel channel, const VIATime *sendTime, uint32 packetSize,
                                          const uint8 *packetData) const = 0;

#endif

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 13)
    // allow to iterate over the network's Ethernet nodes
    VIDB_DECLARE_ITERATABLE(EthernetNode);
#endif

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 14)
    // Registration for Ethernet packets
    VIASTDDECL CreateEthernetPduRequest(VIARequestHandle *handle,   // the created request handle
                                        VIAOnEthernetPdu *sink,     // callback object
                                        VIAChannel channel,         // channel
                                        VIAOnEthernetPdu::VPduType pduType) = 0; // PDU to received
#endif

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 16)

    VIASTDDECL GetPDUService(VIAPDUService **pduService) = 0;

#endif

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 19)

    VIASTDDECL GetServiceDiscoveryIPv4MulticastEndpoint(bool /*in*/ vlan, uint32 /*in*/ vlanId,
                                                        uint32 * /*out*/ multicastIPv4Address,
                                                        uint16 * /*out*/ multicastPort) = 0;

    VIASTDDECL GetServiceDiscoveryIPv6MulticastEndpoint(bool /*in*/ vlan, uint32 /*in*/ vlanId,
                                                        VIASocketAddress6 * /*out*/ multicastIPv6Address,
                                                        uint16 * /*out*/ multicastPort) = 0;

#endif

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 21)

    VIASTDDECL GetEthernetPacketCAPLData(void *caplEthernetPacket, unsigned char *buffer, uint32 bufferLength,
                                         uint32 *copiedLength) = 0;

#endif

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 22)

    /**
       Check whether there is information available to validate the given SOME/IP-Message (currently only SecOC-Validation)
       and to perform the validation if this is the case.

       @param  someIpMessage     (in)  - SOME/IP message which should be validated (if applicable).
       @param  serviceInstanceId (in)  - Service Instance ID that SOME/IP message is related to.
       @param  validationState   (out) - Result of the validation (if any, otherwise ValidationState::kUnsecured).
       @return kVIA_OK in case of success, otherwise an error code.
     */
    VIASTDDECL ValidateMessage(VIASomeIpMessage * /* in */ someIpMessage, uint16 /* in */ serviceInstanceId,
                               VIASomeIpMessage::ValidationState * /* out */ validationState) = 0;

#endif // VIAEthernetMajorVersion >= 1 && VIAEthernetMinorVersion >= 22


#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 23)

    VIASTDDECL GetServiceDiscoveryMulticastEndpoint(bool /*in*/ vlan, uint32 /*in*/ vlanId, VIAEndpointAddr &endpoint,
                                                    VIAAddressFamily wantedFamily = VIA_AF_UNDEF) const = 0;

#endif


#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 27)
    // Registration for Ethernet packets
    VIASTDDECL CreateThisNodePacketRequest(VIARequestHandle *handle,   // the created request handle
                                           VIAOnEthernetPacket *sink,  // callback object
                                           VIAChannel channel) = 0;   // channel

    // allow to iterate over the node's ports
    typedef VIDBIterator<VIAEthernetPort> VIAEthernetPortIterator;

    VIASTDDECL GetNodesEthernetPortIterator(VIAEthernetPortIterator **iterator) = 0;

    VIASTDDECL ConvertCAPLIPEndpointToVIAEndpointAddr(void *caplIP_Endpoint, VIAEndpointAddr &viaEndpointAddr) = 0;

    VIASTDDECL ConvertCAPLIPAddressToVIASockAddr(void *caplIP_Address, VIASockAddr &viaSocketAddr) = 0;

    VIASTDDECL ConvertVIAEndpointAddrToCAPLIPEndpoint(VIAEndpointAddr &viaEndpointAddr, void *caplIP_Endpoint) = 0;

    VIASTDDECL ConvertVIASockAddrToCAPLIPAddress(VIASockAddr &viaSocketAddr, void *caplIP_Address) = 0;

    VIASTDDECL CreateCAPLIPEndpoint(void *&caplIP_Endpoint) = 0;

    VIASTDDECL CreateCAPLIPAddress(void *&caplIP_Address) = 0;

    VIASTDDECL ReleaseCAPLIPEndpoint(void *caplIP_Endpoint) = 0;

    VIASTDDECL ReleaseCAPLIPAddress(void *caplIP_Address) = 0;

#endif


#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 28)

    VIASTDDECL IsSwitchedNetwork(bool &isSwitchedNetwork) = 0;

    // Returns the current received packet's ethernet port within VIAOnEthernetPacket::OnEthernetPacket
    VIASTDDECL GetThisFrameEthernetPort(VIAEthernetPort *&port) = 0;

#endif

}; // class VIAEthernetBus

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 8)

class VIDBEthernetNode {
public:
    // Iterates over application endpoints
    VIDB_DECLARE_ITERATABLE(ApplicationEndpoint)
};


class VIAEthernetPort {
public:
    VIABOOLDECL Compare(VIAEthernetPort &otherPort) = 0;

    VIASTDDECL  Release() = 0;
};

class VIDBApplicationEndpoint {
public:
    enum VDiscoveryTechnologyType {
        kDS_Unknown = 0,
        kDS_SomeIp = 1,
        kDS_FsaDynamic = 2,
        kDS_FsaStatic = 3
    };

    enum VDiscoveryTechnologyVersion {
        kDS_Version_Unknown = 0, kDS_Version_1 = 1
    };

    enum VSerializationTechnologyType {
        kSR_Unknown = 0,
        kSR_SomeIp = 1,
        kSR_GoogleProtocolBuffers = 2
    };

    enum VSerializationTechnologyVersion {
        kSR_Version_Unknown = 0, kSR_Version_1 = 1
    };

    enum VRemotingTechnologyType {
        kRT_Unknown = 0,
        kRT_SomeIp = 1,
        kRT_Fsa = 2
    };

    enum VRemotingTechnologyVersion {
        kRT_Version_Unknown = 0, kRT_Version_1 = 1
    };


    enum VDBAddressSource {
        kDBAddressSourceUndefined = 0,
        kDBAddressSourceFixed = 1,
        kDBAddressSourceDHCPv4 = 2,
        kDBAddressSourceDHCPv6 = 3,
        kDBAddressSourceAUTOIP = 4,
        kDBAddressSourceLinkLocal = 5,
        kDBAddressSourceRouterAdvertisement = 6
    };

public:
    VIASTDDECL GetDiscoveryTechnology(uint32 * /*in/out*/ technology, uint32 * /*in/out*/ version) const = 0;

    VIASTDDECL GetSerializationTechnology(uint32 * /*in/out*/ technology, uint32 * /*in/out*/ version) const = 0;

    VIASTDDECL GetRemotingTechnology(uint32 * /*in/out*/ technology, uint32 * /*in/out*/ version) const = 0;

    VIASTDDECL GetIPv4Address(uint32 * /*out*/ address) const = 0;

    VIASTDDECL GetIPv6Address(VIASocketAddress6 * /*out*/ address) const = 0;

    VIASTDDECL GetUDPPort(uint16 * /*out*/ port) const = 0;

    VIASTDDECL GetTCPPort(uint16 * /*out*/ port) const = 0;

public:
    // Iterates over provided service instance
    VIDB_DECLARE_ITERATABLE(ProvidedServiceInstance)
    // Iterates over consumed service instance
    VIDB_DECLARE_ITERATABLE(ConsumedServiceInstance)
    // Iterates over provided event group
    VIDB_DECLARE_ITERATABLE(ProvidedEventGroup)
    // Iterates over consumed event group
    VIDB_DECLARE_ITERATABLE(ConsumedEventGroup)
    // Iterates over PDU transmissions
    VIDB_DECLARE_ITERATABLE_ALIAS(TxPDUTransmission, PDUTransmission)

    VIDB_DECLARE_ITERATABLE_ALIAS(RxPDUTransmission, PDUTransmission)

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 20)
    // Iterates over Remote AEPs
    VIDB_DECLARE_ITERATABLE_ALIAS(OutgoingServerConnectionAddresses, RemoteApplicationEndpoint)

    VIDB_DECLARE_ITERATABLE_ALIAS(IncomingClientConnectionAddresses, RemoteApplicationEndpoint)

#endif // VIAEthernetMajorVersion >= 1 && VIAEthernetMinorVersion >= 20

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 23)

    VIASTDDECL GetVLANId(uint32 * /*out*/ vlanId) const = 0;

    VIASTDDECL GetApplicationEndpointPriority(uint32 * /*out*/ priority) const = 0;

    VIASTDDECL GetNetworkEndpointPriority(uint32 * /*out*/ priority) const = 0;

    VIASTDDECL GetEndpoint(VIAEndpointAddr &endpoint) const = 0;

#endif // VIAEthernetMajorVersion >= 1 && VIAEthernetMinorVersion >= 23

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 26)

    VIASTDDECL GetEndpointCount(uint32 & /*out*/ count) const = 0;

    VIASTDDECL GetEndpoints(VIAEndpointAddr /*out*/ pEndpoints[], VDBAddressSource /*out*/ pAddressSource[],
                            uint32 /*out*/  pAssignmentPriorities[], uint32 & /*in-out*/ size) const = 0;

#endif // VIAEthernetMajorVersion >= 1 && VIAEthernetMinorVersion >= 26

};


class VIDBRemoteApplicationEndpoint {
public:
    VIASTDDECL GetIPv4Address(uint32 * /*out*/ address) const = 0;

    VIASTDDECL GetIPv6Address(VIASocketAddress6 * /*out*/ address) const = 0;

    VIASTDDECL GetUDPPort(uint16 * /*out*/ port) const = 0;

    VIASTDDECL GetTCPPort(uint16 * /*out*/ port) const = 0;


#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 23)

    VIASTDDECL GetEndpoint(VIAEndpointAddr &endpoint) const = 0;

#endif
};


class VIDBPDUTransmission {
public:
    VIASTDDECL GetSourceRemoteApplicationEndpoint(
            VIDBRemoteApplicationEndpoint ** /*out*/ sourceRemoteApplicationEndpoint) = 0;

    VIASTDDECL GetDestinationRemoteApplicationEndpoint(
            VIDBRemoteApplicationEndpoint ** /*out*/ dstRemoteApplicationEndpoint) = 0;

    VIASTDDECL IsTriggered(bool *isTriggered) const = 0;

    VIASTDDECL GetPDU(VIDBPDUDefinition ** /*out*/ pdu) = 0;

    VIASTDDECL GetContainerPDU(VIDBPDUDefinition ** /*out*/ containerPDU) = 0;

    VIASTDDECL GetProvidedEventGroupHandler(VIDBProvidedEventGroup ** /*out*/ peg) = 0;

    VIASTDDECL GetConsumedEventGroup(VIDBConsumedEventGroup ** /*out*/ ceg) = 0;
};


class VIDBProvidedServiceInstance {
public:
    VIASTDDECL GetServiceInstance(VIDBServiceInstance ** /*out*/ serviceInstance) const = 0;

    VIASTDDECL GetTTL(uint32 * /*out*/ ttlInSec) const = 0;

    VIASTDDECL GetInitialMinDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetInitialMaxDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetInitialRepetitionsMax(uint32 * /*out*/ initialRepetitionsMax) const = 0;

    VIASTDDECL GetInitialRepetitionBaseDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetAnnounceCyclicDelay(uint32 * /*out*/ delayInMilliSec) const = 0;

    VIASTDDECL GetQueryResponseMinDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetQueryResponseMaxDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 23)

    VIASTDDECL GetPriority(uint32 * /*out*/ priority) const = 0;

#endif
};

class VIDBServiceInstance {
public:
    VIASTDDECL GetServiceInterface(VIDBServiceInterface ** /*out*/ serviceInterface) const = 0;

    VIASTDDECL GetInstanceIdentifier(uint32 * /*out*/ identifier) const = 0;
};

class VIDBConsumedServiceInstance {
public:
    VIASTDDECL GetServiceInstance(VIDBServiceInstance ** /*out*/ serviceInstance) const = 0;

    VIASTDDECL GetTTL(uint32 * /*out*/ ttlInSec) const = 0;

    VIASTDDECL GetInitialMinDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetInitialMaxDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetInitialRepetitionsMax(uint32 * /*out*/ initialRepetitionsMax) const = 0;

    VIASTDDECL GetInitialRepetitionBaseDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetRequestCyclicDelay(uint32 * /*out*/ delayInMilliSec) const = 0;

    VIASTDDECL GetQueryResponseMinDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetQueryResponseMaxDelay(uint32 * /*out*/ delayInMicroSec) const = 0;
};

class VIDBProvidedEventGroup {
public:
    VIASTDDECL GetServiceInstance(VIDBServiceInstance ** /*out*/ serviceInstance) const = 0;

    VIASTDDECL GetEventGroup(VIDBEventGroup ** /*out*/ eventGroup) const = 0;

    VIASTDDECL GetTTL(uint32 * /*out*/ ttlInSec) const = 0;

    VIASTDDECL GetInitialMinDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetInitialMaxDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetInitialRepetitionsMax(uint32 * /*out*/ initialRepetitionsMax) const = 0;

    VIASTDDECL GetInitialRepetitionBaseDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetAnnounceCyclicDelay(uint32 * /*out*/ delayInMilliSec) const = 0;

    VIASTDDECL GetQueryResponseMinDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetQueryResponseMaxDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetIPv4MulticastAddress(uint32 * /*out*/ address) const = 0;

    VIASTDDECL GetIPv4UDPMulticastPort(uint16 * /*out*/ port) const = 0;

    VIASTDDECL GetIPv6MulticastAddress(VIASocketAddress6 * /*out*/ address) const = 0;

    VIASTDDECL GetIPv6UDPMulticastPort(uint16 * /*out*/ port) const = 0;

    VIASTDDECL GetMulticastThreshold(uint32 * /*out*/ threshold) const = 0;

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 23)

    VIASTDDECL GetMulticastEndpoint(VIAEndpointAddr & /*out*/ endpoint,
                                    VIAAddressFamily wantedFamily = VIA_AF_UNDEF) const = 0;

#endif
};

class VIDBConsumedEventGroup {
public:
    VIASTDDECL GetServiceInstance(VIDBServiceInstance ** /*out*/ serviceInstance) const = 0;

    VIASTDDECL GetEventGroup(VIDBEventGroup ** /*out*/ eventGroup) const = 0;

    VIASTDDECL GetTTL(uint32 * /*out*/ ttlInSec) const = 0;

    VIASTDDECL GetInitialMinDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetInitialMaxDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetInitialRepetitionsMax(uint32 * /*out*/ initialRepetitionsMax) const = 0;

    VIASTDDECL GetInitialRepetitionBaseDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetRequestCyclicDelay(uint32 * /*out*/ delayInMilliSec) const = 0;

    VIASTDDECL GetQueryResponseMinDelay(uint32 * /*out*/ delayInMicroSec) const = 0;

    VIASTDDECL GetQueryResponseMaxDelay(uint32 * /*out*/ delayInMicroSec) const = 0;
};

class VIDBServiceInterface {
public:
    VIASTDDECL GetName(char * /*out*/ name, uint32 * /*in/out*/ nameLength) const = 0;

    VIASTDDECL GetFullName(char * /*out*/ fullName, uint32 * /*in/out*/ fullNameLength) const = 0;

    VIASTDDECL GetIdentifier(uint32 * /*out*/ identifier) const = 0;

    VIASTDDECL GetApiVersion(uint32 * /*out*/ majorVersion, uint32 * /*out*/ minorVersion) const = 0;

public:
    // Iterates over methods
    VIDB_DECLARE_ITERATABLE(Method)
    // Iterates over events
    VIDB_DECLARE_ITERATABLE(Event)
    // Iterates over fields
    VIDB_DECLARE_ITERATABLE(Field)
    // Iterates over event groups
    VIDB_DECLARE_ITERATABLE(EventGroup)
};

class VIDBMethod {
public:
    enum VCallSemantic {
        kCS_Asynchronous = 1, kCS_FireAndForget = 2
    };

public:
    VIASTDDECL GetName(char * /*out*/ name, uint32 * /*in/out*/ nameLength) const = 0;

    VIASTDDECL GetIdentifier(uint32 * /*out*/ identifier) const = 0;

    VIASTDDECL GetCallSemantic(uint32 * /*out*/ callSemantic) const = 0;

    VIASTDDECL IsReliable(bool * /*out*/ isReliable) const = 0;

    VIASTDDECL GetE2EProtectionInfo(VIASomeIpMessage::VMessageType /*in*/ msgType,
                                    uint32 * /*out*/ e2eProtectionType, uint32 * /*out*/ crcId,
                                    char * /*out*/ suffix, uint32 * /*in/out*/ suffixLength) const = 0;

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 12)
    VIDB_DECLARE_ITERATABLE(InputParameter)

    VIDB_DECLARE_ITERATABLE(OutputParameter)

#endif // VIAEthernetMajorVersion >= 1 && VIAEthernetMinorVersion >= 12
};

class VIDBField {
public:
    VIASTDDECL GetName(char * /*out*/ name, uint32 * /*in/out*/ nameLength) const = 0;

    VIASTDDECL GetNotifierIdentifier(uint32 * /*out*/ identifier) const = 0;

    VIASTDDECL IsNotifierReliable(bool * /*out*/ isNotifierReliable) const = 0;

    VIASTDDECL GetGetterIdentifier(uint32 * /*out*/ identifier) const = 0;

    VIASTDDECL IsGetterReliable(bool * /*out*/ isGetterReliable) const = 0;

    VIASTDDECL GetSetterIdentifier(uint32 * /*out*/ identifier) const = 0;

    VIASTDDECL IsSetterReliable(bool * /*out*/ isSetterReliable) const = 0;

    VIASTDDECL GetE2EProtectionInfo(uint32 * /*out*/ e2eProtectionType, uint32 * /*out*/ crcId,
                                    char * /*out*/ suffix, uint32 * /*in/out*/ suffixLength) const = 0;

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 10)

    VIASTDDECL GetFieldNotifier(VIDBFieldNotifier ** /*out*/ fieldNotifier) const = 0;

#endif

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 12)

    VIASTDDECL GetDataType(VIDBDataType ** /*out*/ dataType) const = 0;

#endif // VIAEthernetMajorVersion >= 1 && VIAEthernetMinorVersion >= 12

};

class VIDBEventGroup {
public:
    VIASTDDECL GetName(char * /*out*/ name, uint32 * /*in/out*/ nameLength) const = 0;

    VIASTDDECL GetIdentifier(uint32 * /*out*/ identifier) const = 0;

public:
    // Iterates over events
    VIDB_DECLARE_ITERATABLE(Event)
    // Iterates over fields
    VIDB_DECLARE_ITERATABLE(Field)
};

#endif

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 10)

class VIDBEvent {
public:
    enum VDataIdMode {
        kDM_All16Bit = 0,
        kDM_Alternating8Bit = 1,
        kDM_Lower8Bit = 2,
        kDM_Lower12Bit = 3
    };

public:
    VIASTDDECL GetName(char * /*out*/ name, uint32 * /*in/out*/ nameLength) const = 0;

    VIASTDDECL GetIdentifier(uint32 * /*out*/ identifier) const = 0;

    VIASTDDECL GetCallSemantic(uint32 * /*out*/ callSemantic) const = 0;

    VIASTDDECL IsReliable(bool * /*out*/ isReliable) const = 0;

    VIASTDDECL GetE2EProtectionInfo(VIASomeIpMessage::VMessageType /*in*/ msgType,
                                    uint32 * /*out*/ e2eProtectionType, uint32 * /*out*/ crcId,
                                    char * /*out*/ suffix, uint32 * /*in/out*/ suffixLength) const = 0;

    VIASTDDECL GetApplicationCycleInMs(uint32 * /*out*/ applicationCycleInMs) const = 0;

    VIASTDDECL GetDebounceTimeDurationInMs(uint32 * /*out*/ debounceTimeDurationInMs) const = 0;

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 12)
    VIDB_DECLARE_ITERATABLE(InputParameter)

    VIDB_DECLARE_ITERATABLE(OutputParameter)

#endif // VIAEthernetMajorVersion >= 1 && VIAEthernetMinorVersion >= 12

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 17)

    VIASTDDECL GetE2EDataOffset(uint32 * /*out*/ dataOffset) const = 0;

    VIASTDDECL GetE2EMaxDeltaCounterInit(uint32 * /*out*/ maxDeltaCounterInit) const = 0;

    VIASTDDECL GetE2EMaxNoNewOrRepeatedData(uint32 * /*out*/ maxNoNewOrRepeatedData) const = 0;

    VIASTDDECL GetE2ESyncCounterInit(uint32 * /*out*/ syncCounterInit) const = 0;

    VIASTDDECL GetE2ECounterOffset(uint32 * /*out*/ counterOffset) const = 0;

    VIASTDDECL GetE2ECrcOffset(uint32 * /*out*/ crcOffset) const = 0;

    VIASTDDECL GetE2EDataIdMode(uint32 * /*out*/ dataIdMode) const = 0;

    VIASTDDECL GetE2EDataIdNibbleOffset(uint32 * /*out*/ dataIdNibbleOffset) const = 0;

    VIASTDDECL GetE2EDataLength(uint32 * /*out*/ dataLength) const = 0;

    VIASTDDECL GetE2EMinDataLength(uint32 * /*out*/ minDataLength) const = 0;

    VIASTDDECL GetE2EMaxDataLength(uint32 * /*out*/ maxDataLength) const = 0;

    VIASTDDECL GetE2EMinOkStateInit(uint32 * /*out*/ minOkStateInit) const = 0;

    VIASTDDECL GetE2EMinOkStateValid(uint32 * /*out*/ minOkStateValid) const = 0;

    VIASTDDECL GetE2EMinOkStateInvalid(uint32 * /*out*/ minOkStateInvalid) const = 0;

    VIASTDDECL GetE2EMaxErrorStateInit(uint32 * /*out*/ maxErrorStateInit) const = 0;

    VIASTDDECL GetE2EMaxErrorStateValid(uint32 * /*out*/ maxErrorStateValid) const = 0;

    VIASTDDECL GetE2EMaxErrorStateInvalid(uint32 * /*out*/ maxErrorStateInvalid) const = 0;

    VIASTDDECL GetE2EUpperHeaderBitsToShift(uint32 * /*out*/ upperHeaderBitsToShift) const = 0;

    VIASTDDECL GetE2EWindowSize(uint32 * /*out*/ windowSize) const = 0;

    VIASTDDECL GetE2ECategory(char *category, uint32 *categoryLength) const = 0;

    VIASTDDECL GetE2EDataIDs(char *dataIds, uint32 *dataIdsLength) const = 0;

#endif // VIAEthernetMajorVersion >= 1 && VIAEthernetMinorVersion >= 17

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 18)

    VIASTDDECL IsTriggeredOnSubscription(bool *triggeredOnSubscription) const = 0;

#endif // VIAEthernetMajorVersion >= 1 && VIAEthernetMinorVersion >= 18
#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 29)

    VIASTDDECL GetNetworkCycleInMs(uint32 * /*out*/ networkCycleInMs) const = 0;

#endif

};

class VIDBFieldNotifier {
public:
    VIASTDDECL GetIdentifier(uint32 * /*out*/ identifier) const = 0;

    VIASTDDECL IsReliable(bool * /*out*/ isNotifierReliable) const = 0;

    VIASTDDECL GetApplicationCycleInMs(uint32 * /*out*/ applicationCycleInMs) const = 0;

    VIASTDDECL GetDebounceTimeDurationInMs(uint32 * /*out*/ debounceTimeDurationInMs) const = 0;

#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 29)

    VIASTDDECL GetNetworkCycleInMs(uint32 * /*out*/ networkCycleInMs) const = 0;

#endif
};

#endif


#if defined (VIAEthernetMajorVersion) && (VIAEthernetMajorVersion >= 1) && (VIAEthernetMinorVersion) && (VIAEthernetMinorVersion >= 12)

class VIDBDataType {
public:
    typedef enum {
        VIDBDataTypeInterpretation_Unknown = 0,
        VIDBDataTypeInterpretation_Integer = 1,
        VIDBDataTypeInterpretation_Float = 2,
        VIDBDataTypeInterpretation_UnicodeString = 3,
        VIDBDataTypeInterpretation_ByteField = 4,
        VIDBDataTypeInterpretation_BitField = 5,
        VIDBDataTypeInterpretation_Boolean = 6,
        VIDBDataTypeInterpretation_Enum = 7,
        VIDBDataTypeInterpretation_NULL = 8,
        VIDBDataTypeInterpretation_Struct = 9,
        VIDBDataTypeInterpretation_Union = 10,
        VIDBDataTypeInterpretation_Array = 11,
        VIDBDataTypeInterpretation_TypeDef = 12,
        VIDBDataTypeInterpretation_AsciiString = 13
    } VIDBDataTypeInterpretation;

public:
    VIASTDDECL GetName(char * /*out*/ name, uint32 * /*in/out*/ nameLength) const = 0;

    VIASTDDECL GetDataTypeInterpretation(uint32 * /*out*/ datatypeInterpretation) const = 0;

    VIASTDDECL IsArray(bool * /*out*/ isArray) const = 0;

    VIASTDDECL GetPosition(bool * /*out*/ hasPosition, uint32 * /*out*/ position) const = 0;

    VIASTDDECL GetBitLength(bool * /*out*/ hasBitLength, uint32 * /*out*/ bitLength) const = 0;

    VIASTDDECL IsSigned(bool * /*out*/ hasSignedInformation, bool * /*out*/ isSigned) const = 0;

public:
    // Iterates over children
    VIDB_DECLARE_ITERATABLE(DataType)
    // Iterate over array dimensions
    VIDB_DECLARE_ITERATABLE(ArrayDimension)
};

class VIDBArrayDimension {
public:
    VIASTDDECL GetDimension(uint32 * /*out*/ dimension) const = 0;

    VIASTDDECL GetMiniumSize(uint32 * /*out*/ minimumSize) const = 0;

    VIASTDDECL GetMaximumSize(uint32 * /*out*/ maximumSize) const = 0;
};

/**
 * NOTE: Despite equal, both classes VIDBInputParameter and VIDBOutputParameter are required in order to use
 *       the macros VIDB_DECLARE_ITERATABLE(InputParameter) and VIDB_DECLARE_ITERATABLE(OutputParameter)
 */
class VIDBInputParameter {
public:
    VIASTDDECL GetName(char * /*out*/ name, uint32 * /*in/out*/ nameLength) const = 0;

    VIASTDDECL IsMandatory(bool * /*out*/ isMandatory) const = 0;

    VIASTDDECL GetPosition(uint32 * /*out*/ position) const = 0;

    VIASTDDECL GetDataType(VIDBDataType ** /*out*/ dataType) const = 0;
};

/**
 * NOTE: Despite equal, both classes VIDBInputParameter and VIDBOutputParameter are required in order to use
 *       the macros VIDB_DECLARE_ITERATABLE(InputParameter) and VIDB_DECLARE_ITERATABLE(OutputParameter)
 */
class VIDBOutputParameter {
public:
    VIASTDDECL GetName(char * /*out*/ name, uint32 * /*in/out*/ nameLength) const = 0;

    VIASTDDECL IsMandatory(bool * /*out*/ isMandatory) const = 0;

    VIASTDDECL GetPosition(uint32 * /*out*/ position) const = 0;

    VIASTDDECL GetDataType(VIDBDataType ** /*out*/ dataType) const = 0;
};

#endif // VIAEthernetMajorVersion >= 1 && VIAEthernetMinorVersion >= 12

#endif // VIA_ETHERNET_H