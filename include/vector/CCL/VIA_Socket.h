/*----------------------------------------------------------------------------
|
|  File Name: VIA_Socket.h 
|
|  Comment:   External interface for sockets (for Nodelayer-DLLs)
|                                         
|-----------------------------------------------------------------------------
|               A U T H O R   I D E N T I T Y
|-----------------------------------------------------------------------------
| Initials     Name                      Company
| --------     ---------------------     ------------------------------------
| Sn           Michael Stephan           Vector Informatik GmbH
|-----------------------------------------------------------------------------
|               R E V I S I O N   H I S T O R Y
|-----------------------------------------------------------------------------
| Date         Ver  Author  Description
| ----------   ---  ------  --------------------------------------------------
| 13.11.2008   1.0  Sn      Creation
| 31.01.2014   1.5  Jhn     added new interface VIASocketServiceEx
| 29.04.2014   1.6  Jhn     added new methods to VIASocketService
| 17.09.2014   1.7  Sbj     added new method  to VIASocket
| 22.01.2016   1.8  Jhn     added setter and getter for stack parameters
| 12.10.2017   1.9  Jhn     added new addresses and new interface version
| 13.02.2018   1.9  Jhn     added defines for socket options
| 08.06.2018   1.10 Jhn     added VIATlsSocket
| 27.11.2018   1.11 Jhn     added VIAAdapterCallback interface
| 20.02.2020   1.12 Jhn     added VIATlsSocketServiceClient2 callback interface
|-----------------------------------------------------------------------------
|               C O P Y R I G H T
|-----------------------------------------------------------------------------
| Copyright (c) 2008 by Vector Informatik GmbH.  All rights reserved.
-----------------------------------------------------------------------------*/


#ifndef VIA_SOCKET_H
#define VIA_SOCKET_H

#ifndef VIA_H
#include "VIA.h"
#endif


// ============================================================================
// Version of the interfaces, which are declared in this header
// See  REVISION HISTORY for a description of the versions
// ============================================================================
#if !defined ( VIASocketMajorVersion )
#define VIASocketMajorVersion 1
#endif
#if !defined ( VIASocketMinorVersion )
#define VIASocketMinorVersion 12
#endif


// ============================================================================
// supported socket options 
// ============================================================================
// socket option levels
#define VIA_SOL_IP                    0x00000000
#define VIA_SOL_TCP                   0x00000006
#define VIA_SOL_UDP                   0x00000011
#define VIA_SOL_IPV6                  0x00000029
#define VIA_SOL_SOCKET                0x0000FFFF
#define VIA_SOL_VECTOR                0x00010002

// socket-level options.
#define VIA_SO_DEBUG                  0x00000001
#define VIA_SO_ACCEPTCONN             0x00000002
#define VIA_SO_REUSEADDR              0x00000004
#define VIA_SO_KEEPALIVE              0x00000008
#define VIA_SO_DONTROUTE              0x00000010
#define VIA_SO_BROADCAST              0x00000020
#define VIA_SO_USELOOPBACK            0x00000040
#define VIA_SO_LINGER                 0x00000080
#define VIA_SO_OOBINLINE              0x00000100
#define VIA_SO_SNDBUF                 0x00001001
#define VIA_SO_RCVBUF                 0x00001002
#define VIA_SO_SNDLOWAT               0x00001003
#define VIA_SO_RCVLOWAT               0x00001004
#define VIA_SO_SNDTIMEO               0x00001005
#define VIA_SO_RCVTIMEO               0x00001006
#define VIA_SO_ERROR                  0x00001007
#define VIA_SO_TYPE                   0x00001008

// ip-level options
#define VIA_IP_OPTIONS                0x00000001
#define VIA_IP_HDRINCL                0x00000002
#define VIA_IP_TOS                    0x00000003
#define VIA_IP_TTL                    0x00000004
#define VIA_IP_MULTICAST_IF           0x00000009
#define VIA_IP_MULTICAST_TTL          0x0000000A
#define VIA_IP_MULTICAST_LOOP         0x0000000B
#define VIA_IP_ADD_MEMBERSHIP         0x0000000C
#define VIA_IP_DROP_MEMBERSHIP        0x0000000D
#define VIA_IP_DONTFRAGMENT           0x0000000E
#define VIA_IP_ADD_SOURCE_MEMBERSHIP  0x0000000F
#define VIA_IP_DROP_SOURCE_MEMBERSHIP 0x00000010
#define VIA_IP_BLOCK_SOURCE           0x00000011
#define VIA_IP_UNBLOCK_SOURCE         0x00000012
#define VIA_IP_ONESBCAST              0x00000017
#define VIA_IP_RECVIF                 0x00000018

// ipv6-level options
#define VIA_IPV6_HOPOPTS              0x00000001
#define VIA_IPV6_UNICAST_HOPS         0x00000004
#define VIA_IPV6_MULTICAST_IF         0x00000009
#define VIA_IPV6_MULTICAST_HOPS       0x0000000A
#define VIA_IPV6_MULTICAST_LOOP       0x0000000B
#define VIA_IPV6_JOIN_GROUP           0x0000000C
#define VIA_IPV6_LEAVE_GROUP          0x0000000D
#define VIA_IPV6_DONTFRAG             0x0000000E
#define VIA_IPV6_PKTINFO              0x00000013
#define VIA_IPV6_HOPLIMIT             0x00000015
#define VIA_IPV6_CHECKSUM             0x0000001A
#define VIA_IPV6_V6ONLY               0x0000001B
#define VIA_IPV6_RTHDR                0x00000020
#define VIA_IPV6_RECVRTHDR            0x00000026
#define VIA_IPV6_TCLASS               0x00000027
#define VIA_IPV6_RECVTCLASS           0x00000028

// new socket options introduced in RFC3542
#define VIA_IPV6_USE_MIN_MTU          0x0029002A

// tcp-level options
#define VIA_TCP_NODELAY               0x00000001
#define VIA_TCP_MAXSEG                0x00000004
#define VIA_TCP_NOPUSH                0x00060004
#define VIA_TCP_NOOPT                 0x00060008

// protocol independent multicast source filter options
#define VIA_MCAST_JOIN_GROUP          0x00000029
#define VIA_MCAST_LEAVE_GROUP         0x0000002A
#define VIA_MCAST_BLOCK_SOURCE        0x0000002B
#define VIA_MCAST_UNBLOCK_SOURCE      0x0000002C
#define VIA_MCAST_JOIN_SOURCE_GROUP   0x0000002D
#define VIA_MCAST_LEAVE_SOURCE_GROUP  0x0000002E

// vector-level options
#define VIA_SO_VLANPRIO               0x00010001

// ============================================================================
// struct VIASocketAddress6
// ============================================================================
struct VIASocketAddress6 {
    union {
        uint8 Byte[16];
        uint16 Word[8];
    };
};

// ============================================================================
// The following structs are for the generic use of the different types of 
// socket addresses used in VIA_SOCKET2 interface
// ============================================================================
typedef enum VIAAddressFamily {
    VIA_AF_UNDEF = 0,  // address family not defined
    VIA_AF_INET = 2,  // IPv4 address family
    VIA_AF_LINK = 18, // ethernet address family
    VIA_AF_INET6 = 28  // IPv6 address family
} VIAAddressFamily;

enum VIAProtcolType : uint16 {
    kVIA_IPPROTO_TCP = 6, // tcp protocol
    kVIA_IPPROTO_UDP = 17 // udp protocol
};

#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 9)
#pragma pack(push, 1)

// address given in network byte order
struct VIAAddressEthernet {
    struct bytes {
        uint8 b1, b2, b3, b4, b5, b6;
    };

    union {
        bytes b;
        uint8 byteAddress[6];
        uint16 wordAddress[3];
    };
};

// address given in network byte order
struct VIAAddressIPv4 {
    struct bytes {
        uint8 b1, b2, b3, b4;
    };

    union {
        bytes b;
        uint8 byteAddress[4];
        uint16 wordAddress[2];
        uint32 longAddress;
    };
};

// address given in network byte order
struct VIAAddressIPv6 {
    union {
        VIASocketAddress6 addrOld;
        uint8 byteAddress[16];
        uint16 wordAddress[8];
        uint32 longAddress[4];
    };
};

struct VIASockAddrEth {
    VIAAddressEthernet address;   // mac address in network byte order
    uint8 reserved[24]; // set to zero!
};

struct VIASockAddrIPv4 {
    VIAAddressIPv4 address;       // ipv4 address in network byte order
    uint8 reserved[26];  // set to zero!
};

struct VIASockAddrIPv6 {
    VIAAddressIPv6 address;        // ipv6 address in network byte order
    uint32 flowinfo;       // ipv6 flow info
    uint32 scopeId;        // ipv6 scope id or zero for automatic scope
    uint8 reserved[6];    // set to zero!
};


struct VIASockAddr {
    VIAAddressFamily addressFamily;
    union {
        VIASockAddrEth addressEth;    // ethernet address
        VIASockAddrIPv4 addressIPv4;   // IPv4 address
        VIASockAddrIPv6 addressIPv6;   // IPv6 address
        uint8 rawData[30];  // raw data array
    };
};

struct VIAEndpointAddr
        : public VIASockAddr {
    uint16 port;         // transport layer port in host byte order
    VIAProtcolType protocolType; // transport layer protocol type
    int8 reserved[12]; // reserved -> set to zero!
};

// network address defined by address and prefix
struct VIANetworkAddr
        : public VIASockAddr {
    union {
        uint16 ethInterfaceIndex; // interface index or zero for automatic index
        uint32 ipv6Prefix; // prefix for ipv6 addresses
        VIAAddressIPv4 ipv4Mask;   // mask for ipv4 addresses
    };
    int8 reserved[12];   // reserved -> set to zero!
};

#pragma pack(pop)


// ============================================================================
// class VIASocket2
// ============================================================================
class VIASocket2 {
public:
    // open a socket. The type depends on the given address family and protocol type in the address
    VIASTDDECL Open(const VIAEndpointAddr *pAddress) = 0;

    // bind a socket
    VIASTDDECL Bind(const VIAEndpointAddr *pAddress) = 0;

    // start listening on a tcp server socket
    VIASTDDECL Listen() = 0;

    // connect a socket to a server socket
    VIASTDDECL Connect(const VIAEndpointAddr *pAddress) = 0;

    // accept an incoming connection -> creates a new socket for the current connection
    VIASTDDECL Accept(VIASocket2 **ppSocket) = 0;

    // send data on a connected socket
    VIASTDDECL Send(void *pParam, int8 buffer[], uint32 size) = 0;

    // send data on a unconnected socket
    VIASTDDECL SendTo(void *pParam, const VIAEndpointAddr *pAddress, int8 buffer[], uint32 size) = 0;

    // receive data
    VIASTDDECL Receive(void *pParam, int8 buffer[], uint32 bufsize) = 0;

    // shutdown a connected socket
    VIASTDDECL Shutdown() = 0;

    // close a socket
    VIASTDDECL Close() = 0;

    // get the last error code occured on this socket
    VIASTDDECL GetLastError(int32 *error) = 0;

    // get a textual interpretation of the last socket error
    VIASTDDECL GetLastErrorAsString(int8 buffer[], uint32 size) = 0;

    // get the value of a socket option
    VIASTDDECL GetOption(int32 level, int32 name, int32 *value) = 0;

    // set the value of a socket option
    VIASTDDECL SetOption(int32 level, int32 name, int32 value) = 0;

    // set the value of s socket option which needs a struct
    VIASTDDECL SetOptionEx(int32 level, int32 name, int8 value[], int32 length) = 0;

    // get the address of the remote endpoint on a connected socket
    VIASTDDECL GetRemoteAddress(VIAEndpointAddr *pAddress) = 0;

    // get the local address and port, the socket is bound to
    VIASTDDECL GetSocketName(VIAEndpointAddr *pAddress) = 0;

    // get the address family of the socket
    VIASTDDECL GetAddressFamily(VIAAddressFamily *afFamily) = 0;

    // join a multicast group
    VIASTDDECL JoinMulticastGroup(const VIASockAddr *pAddress, uint32 ifIndex) = 0;

    // leave a multicast group
    VIASTDDECL LeaveMulticastGroup(const VIASockAddr *pAddress, uint32 ifIndex) = 0;

    // set the interface index for the multicast group
    VIASTDDECL SetMulticastInterface(uint32 ifIndex) = 0;
};


// ============================================================================
// class VIASocketClient
// ============================================================================
class VIASocketClient2 {
public:
    // is called when the socket is ready to send after it returned from send method with VIA_IO_PENDING 
    VIASTDDECL OnSocketSend(VIASocket2 *pSocket, void *pParam, int32 result, int32 protocol, int8 *pBuffer,
                            uint32 size) = 0;

    // is called when data is received after receive method was called
    VIASTDDECL OnSocketReceive(VIASocket2 *pSocket, void *pParam, int32 result, int32 protocol,
                               const VIAEndpointAddr *pAddress, int8 *pBuffer, uint32 size) = 0;

    // is called when a client tries to connect on a listening socket
    VIASTDDECL OnSocketListen(VIASocket2 *pSocket, int32 result) = 0;

    // is called on the client side after connect call is finished
    VIASTDDECL OnSocketConnect(VIASocket2 *pSocket, int32 result) = 0;

    // is called when socket gets closed eg. by remote endpoint
    VIASTDDECL OnSocketClose(VIASocket2 *pSocket, int32 result) = 0;
};

#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 10)

// ============================================================================
// class VIATlsSocketService
// ============================================================================
class VIATlsSocketService {
public:
    enum VIATlsResult {
        kSuccess = 0,
        kFailed = 1,
        kInvalidParameter = 2,
        kSocketError = 3,
        kIoPending = 4,
        kPeerClosed = 5
    };

    // start tls handshake as client
    VIASTDDECL AuthenticateAsClient(VIASocket2 *pSocket, const char *serverName) = 0;

    // start tls handshake as client with a client certificate
    VIASTDDECL AuthenticateAsClient(VIASocket2 *pSocket, const char *serverName, const char *certificate) = 0;

    // start tls handshake as server
    VIASTDDECL AuthenticateAsServer(VIASocket2 *pSocket, const char *certificate, bool verifyClientCertificate) = 0;

    // send a TLS Alert message to the peer. The socket gets closed if closeUnderlyingSocket is set. Otherwise
    // the VIASocket2 stays open without TLS support
    VIASTDDECL CloseTlsSocket(VIASocket2 *pSocket, bool closeUnderlyingSocket) = 0;

    // get the last error code occured on this socket
    VIASTDDECL GetLastTlsError(VIASocket2 *pSocket, VIATlsResult *error) = 0;

    // get a textual interpretation of the last socket error
    VIASTDDECL GetLastTlsErrorAsString(VIASocket2 *pSocket, char buffer[], uint32 bufferLength) = 0;
};

// ============================================================================
// class VIATlsSocketClient
// ============================================================================
class VIATlsSocketServiceClient {
public:
    // is called when the TLS handshake is finished
    VIASTDDECL OnTlsHandshakeFinished(VIASocket2 *pSocket, VIATlsSocketService::VIATlsResult result) = 0;
};

#endif
#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 12)

class VIATlsSocketServiceClient2 {
public:
    // ============================================================================
    // Version of this interfaces
    // See  REVISION HISTORY for a description of the versions
    // 2020-02-18   2.0  jhn    created
    // ============================================================================
    static const int32 kVersionMajor = 2;
    static const int32 kVersionMinor = 0;

    // has to return the version it implements
    VIASTDDECL GetVersion(int32 * /*out*/ major, int32 * /*out*/ minor) const = 0;
    // is called when the TLS handshake is finished
    VIASTDDECL OnTlsHandshakeFinished(VIASocket2 *pSocket, VIATlsSocketService::VIATlsResult result) = 0;
    // is called when a client connects to a dtls server
    VIASTDDECL OnDtlsServerConnect(VIASocket2 *socketHandle, const VIAEndpointAddr *pAddress,
                                   VIATlsSocketService::VIATlsResult result) = 0;
    // is called when the TLS connection is closed by peer
    VIASTDDECL OnTlsClose(VIASocket2 *pSocket, VIATlsSocketService::VIATlsResult result) = 0;
};

#endif

#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 11)

// ============================================================================
// class VIAAdapterEvents
// ============================================================================
class VIAAdapterEvents {
public:
    enum VIAAddressOrigin {
        kVIA_ORIGIN_UNKNOWN = 0,
        kVIA_ORIGIN_USER = 1,
        kVIA_ORIGIN_SYSTEM = 2,
        kVIA_ORIGIN_DHCPv4CLIENT = 3,
        kVIA_ORIGIN_DHCPv6CLIENT = 4,
        kVIA_ORIGIN_RFC3927CLIENT = 5
    };

    VIASTDDECL OnAdapterAddressAdded(uint32 ifIndex, const VIANetworkAddr *address, VIAAddressOrigin origin,
                                     const char *identification) = 0;

    VIASTDDECL OnAdapterAddressRemoved(uint32 ifIndex, const VIANetworkAddr *address, VIAAddressOrigin origin,
                                       const char *identification) = 0;
};

#endif

// ============================================================================
// class VIASocketService
// ============================================================================
class VIASocketService2 {
public:
    // create a new socket
    VIASTDDECL CreateSocket(VIASocketClient2 *pClient, VIASocket2 **ppSocket) = 0;

    // release the given socket
    VIASTDDECL ReleaseSocket(VIASocket2 *pSocket) = 0;

    // get the count of adapters on this network stack instance
    VIASTDDECL GetAdapterCount(uint32 *pCount) = 0;

    // get the count of addresses of the given address family on the given adapter
    VIASTDDECL GetAdapterAddressCount(uint32 /*in*/ ifIndex, uint32 * /*out*/ addrCount,
                                      VIAAddressFamily /*in*/ family = VIA_AF_UNDEF) = 0;

    // get a array of adapter addresses from the interface which is identified by ifIndex 
    // the size of the array is given in parameter size. When the function returns the count of addresses
    // in the array is returned also in the parameter size
    VIASTDDECL GetAdapterAddresses(uint32 /*in*/ ifIndex, VIANetworkAddr /*in*/ pAddress[], uint32 * /*in-out*/ size,
                                   VIAAddressFamily /*in*/ family = VIA_AF_UNDEF) = 0;

    // get the gateway address of the given address family (VIA_AF_INET or VIA_AF_INET6 allowed)
    VIASTDDECL GetAdapterGateway(uint32 /*in*/ ifIndex, VIAAddressFamily /*in*/ family,
                                 VIASockAddr * /*out*/ pAddress) = 0;

    // get a textual description of the adapter 
    VIASTDDECL GetAdapterDescription(uint32 ifIndex, int8 buffer[], uint32 size) = 0;

    // convert a address to its textual interpretation
    VIASTDDECL GetAddressAsString(const VIASockAddr *pAddress, int8 buffer[], uint32 size) = 0;

    // convert a textual address to a socket address
    VIASTDDECL GetAddressFromString(int8 buffer[], VIASockAddr *pAddress) = 0;

    // get the last error occured on this network stack instance
    VIASTDDECL GetLastError(int32 *error) = 0;

    // get the version of the socket interface
    VIASTDDECL GetVersion(int32 *major, int32 *minor) = 0;

#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 10)
    // deprecated use CreateTlsSocketService2
    VIASTDDECL CreateTlsSocketService(VIATlsSocketServiceClient *pClient, VIATlsSocketService **ppTlsService) = 0;

    // release the given TLS socket service
    VIASTDDECL ReleaseTlsSocketService(VIATlsSocketService *pTlsService) = 0;

#endif
#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 12)
    // create a socket service to handle TLS sockets
    VIASTDDECL CreateTlsSocketService2(VIATlsSocketServiceClient2 *pClient, VIATlsSocketService **ppTlsService) = 0;

#endif
};


// ============================================================================
// class VIASocketServiceEx , does not work for Windows-Stack
// ============================================================================
class VIASocketServiceEx2 {
public:
    // returns the application channel for which the adapter has been created
    VIASTDDECL GetAdapterApplicationChannel(uint32 ifIndex, VIAChannel *channel, VIABusInterfaceType *type) = 0;

    // add a IPv4/IPv6 address to the interface identified by ifIndex
    VIASTDDECL AddAdapterAddress(uint32 /*in*/ ifIndex, const VIANetworkAddr /*in*/ *address) = 0;

    // remove a IPv4/IPv6 address from the interface identified by ifIndex
    VIASTDDECL RemoveAdapterAddress(uint32 /*in*/ ifIndex, const VIANetworkAddr /*in*/ *address) = 0;

    // set the IPv4/IPv6 gateway address
    VIASTDDECL SetAdapterGateway(const VIASockAddr /*in*/ *address) = 0;

    // add/change a static route from destination to gateway in the network stack.
    // Its possible to add a host or network route. If there is a prefix set
    // in the destination address a network route will be installed otherwise
    // a host route will be installed.
    // To add a static route to the IP routing tables the destination and
    // gateway addresses has to be from the same type of network address
    // (VIA_AF_INET or VIA_AF_INET6).
    // To install a interface route in the IP routing table the gateway address
    // has to be of type VIA_AF_LINK with a all zero ethernet address but the
    // interface index has to be set.
    // To add a static route to the ARP routing table the destination address
    // has to be of type VIA_AF_INET or VIA_AF_INET6 and the gateway address
    // has to be of type VIA_AF_LINK with a valid ethernet address (!=00:00:00
    // :00:00:00)
    // If the interface index of the gateway is set to zero, the routing command
    // try to find the correct interface automatically. Therefore the command
    // checks if there already exists a route to destination (for instance a
    // network route) and takes the same interface.
    VIASTDDECL AddRoute(const VIANetworkAddr * /*in*/ destination, const VIANetworkAddr * /*in*/ gateway) = 0;

    // delete a route to destination from the network stack
    VIASTDDECL DeleteRoute(const VIANetworkAddr * /*in*/ destination,
                           const VIANetworkAddr * /*in*/ gateway = nullptr) = 0;

    // get the vlan id and the vlan priority of the interface with the given index.
    VIASTDDECL GetAdapterVlanParameter(uint32 /*in*/ ifIndex, uint16 /*out*/ *vlanId, uint16 /*out*/ *vlanPrio) = 0;

    // set a sysctl parameter in the tcp/ip stack by its path and name (e.g. "net.inet.tcp.delayed_ack")
    VIASTDDECL SetStackParameter(const char * /*in*/ parameterName, int32 /*in*/ value) = 0;

    // read a sysctl parameter from the tcp/ip stack by its path and name (e.g. "net.inet.tcp.delayed_ack")
    VIASTDDECL GetStackParameter(const char * /*in*/ parameterName, int32 * /*out*/ value) = 0;

    // set a parameter in the tcp/ip stack for a given interface by its path and name (e.g. "canoe.interface.mtu")
    VIASTDDECL SetInterfaceParameter(uint32 /*in*/ ifIndex, const char * /*in*/ parameterName, int32 /*in*/ value) = 0;

    // read a parameter from the tcp/ip stack for a given interfyce by its path and name (e.g. "canoe.interface.mtu")
    VIASTDDECL GetInterfaceParameter(uint32 /*in*/ ifIndex, const char * /*in*/ parameterName,
                                     int32 * /*out*/ value) = 0;

#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 11)
    // Register the callback interface for adapter events on the given adapter
    VIASTDDECL RegisterAdapterEventCallbacks(uint32 /*in*/ ifIndex, VIAAdapterEvents * /*in*/ pCallbacks) = 0;
    // Remove the callback interfaece for adapter events from the given adapter
    VIASTDDECL UnregisterAdapterEventCallbacks(uint32 /*in*/ ifIndex, VIAAdapterEvents * /*in*/ pCallbacks) = 0;

#endif
};

#endif

//=============================================================================
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// the following interface is obsolete and will not be updated in the future
// use the interface above for new implementations.
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//=============================================================================


// ============================================================================
// address structs
// 
//         !!!!!!!!! obsolete -> use the address types above !!!!!!!!
//
// ============================================================================
#pragma pack(push, 4)

// address struct for ethernet (mac) addresses with optional interface index
struct VIASocketAddressEth {
    uint8 size;             // the size of this struct
    uint16 interfaceIndex;   // interfaceIndex == 0, automatically selected interface index
    uint8 reserved;
    uint8 address[6];       // the ethernet address
};

// address struct for IPv4 addresses with optional interface index
struct VIASocketAddressIPv4 {
    uint8 size;             // the size of this struct
    uint16 interfaceIndex;   // interfaceIndex == 0, automatically selected interface index
    uint8 reserved;
    uint32 address;          // IPv4 address
};

// address struct for IPv6 addresses with optional interface index
struct VIASocketAddressIPv6 {
    uint8 size;             // the size of this struct
    uint16 interfaceIndex;   // interfaceIndex == 0, automatically selected interface index
    uint8 reserved;
    union {
        VIASocketAddress6 sockAddr;
        uint8 byteAddress[16];
        uint16 wordAddress[8];
        uint32 longAddress[4];
    } address;                // IPv6 address
};

// generic address struct for different type of socket addresses
struct VIASocketAddress {
    uint8 size;                         // the size of this struct
    uint8 addressFamily;                // the address family of the following address
    uint16 reserved;
    union {
        VIASocketAddressEth addressEth;
        VIASocketAddressIPv4 addressIPv4;
        VIASocketAddressIPv6 addressIPv6;
    };
};

#define VIA_INIT_SOCKETADDRESS_ADDRSIZE_FOR_VIA_AF_LINK(addr) (addr).addressEth.size = sizeof(VIASocketAddressEth);
#define VIA_INIT_SOCKETADDRESS_ADDRSIZE_FOR_VIA_AF_INET(addr) (addr).addressEth.size = sizeof(VIASocketAddressIPv4);
#define VIA_INIT_SOCKETADDRESS_ADDRSIZE_FOR_VIA_AF_INET6(addr) (addr).addressEth.size = sizeof(VIASocketAddressIPv6);

#define VIA_INIT_SOCKETADDRESS(addr, family) \
  memset(&(addr), 0, sizeof(VIASocketAddress)); \
  (addr).size = sizeof(VIASocketAddress); \
  (addr).addressFamily = family; \
  VIA_INIT_SOCKETADDRESS_ADDRSIZE_FOR_##family(addr)


// address endpoint for higher layer protocols
struct VIAEndpoint {
    uint32 size;                     // the size of this struct
    struct VIASocketAddress address; // the address
    uint16 port;                     // transport layer port
};

#define VIA_INIT_ENDPOINT(addr, family) \
  VIA_INIT_SOCKETADDRESS((addr).address, family); \
  (addr).size = sizeof(struct VIAEndpoint); \
  (addr).port = 0;

// network address defined by address and prefix
struct VIANetworkAddress {
    uint32 size;                     // the size of this struct
    struct VIASocketAddress address; // the address
    uint32 addressPrefix;            // address prefix
};

#define VIA_INIT_NETWORKADDRESS(addr, family) \
  VIA_INIT_SOCKETADDRESS((addr).address, family); \
  (addr).size = sizeof(struct VIANetworkAddress); \
  (addr).addressPrefix = 0;
#pragma pack(pop)

// ============================================================================
// class VIASocket
// 
//                  !!!! obsolete -> use VIASocket2 !!!!!
//
// ============================================================================
class VIASocket {
public:
    // 
    VIASTDDECL GetLastError(int32 *error) = 0;
    // 
    VIASTDDECL GetLastErrorAsString(int8 buffer[], uint32 size) = 0;
    // 
    VIASTDDECL GetOption(int32 level, int32 name, int32 *value) = 0;
    // 
    VIASTDDECL SetOption(int32 level, int32 name, int32 value) = 0;
    // 
    VIASTDDECL Bind(uint32 address, uint16 port) = 0;
    // 
    VIASTDDECL Bind(const VIASocketAddress6 *pAddress, uint16 port) = 0;
    // 
    VIASTDDECL Open(uint32 address, uint16 port) = 0;
    // 
    VIASTDDECL Open(const VIASocketAddress6 *pAddress, uint16 port) = 0;
    // 
    VIASTDDECL Close() = 0;
    // 
    VIASTDDECL SendTo(void *pParam, uint32 address, uint16 port, int8 buffer[], uint32 size) = 0;
    // 
    VIASTDDECL SendTo(void *pParam, const VIASocketAddress6 *pAddress, uint16 port, int8 buffer[], uint32 size) = 0;
    // 
    VIASTDDECL ReceiveFrom(void *pParam, int8 buffer[], uint32 size) = 0;
    // 
    VIASTDDECL Send(void *pParam, int8 buffer[], uint32 size) = 0;
    // 
    VIASTDDECL Receive(void *pParam, int8 buffer[], uint32 size) = 0;
    // 
    VIASTDDECL Listen() = 0;
    // 
    VIASTDDECL Accept(VIASocket **ppSocket) = 0;
    // 
    VIASTDDECL Connect(uint32 address, uint16 port) = 0;
    // 
    VIASTDDECL Connect(const VIASocketAddress6 *pAddress, uint16 port) = 0;
    // 
    VIASTDDECL Shutdown() = 0;

#if defined ( VIAMinorVersion) && (VIAMinorVersion >= 45)
#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 1)
    // 
    VIASTDDECL GetRemoteAddress(uint32 *pAddress) = 0;
    // 
    VIASTDDECL GetRemoteAddress(VIASocketAddress6 *pAddress) = 0;

#endif
#endif
#if defined ( VIAMinorVersion) && (VIAMinorVersion >= 49)
#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 2)
    // 
    VIASTDDECL SetOptionEx(int32 level, int32 name, char value[], int32 length) = 0;

#endif
#endif
#if defined ( VIAMinorVersion) && (VIAMinorVersion >= 49)
#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 3)
    // get the IPv4 address and the port asigned to a socket
    VIASTDDECL GetSockName(uint32 *pAddress, uint16 *port) = 0;
    // get the IPv6 address and the port assigned to a socket
    VIASTDDECL GetSockName(VIASocketAddress6 *pAddress, uint16 *port) = 0;
    //
#endif
#endif
#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 4)

    VIASTDDECL JoinMulticastGroup(uint32 address, uint32 ifIndex) = 0;

    VIASTDDECL JoinMulticastGroup(const VIASocketAddress6 *pAddress, uint32 ifIndex) = 0;

    VIASTDDECL LeaveMulticastGroup(uint32 address, uint32 ifIndex) = 0;

    VIASTDDECL LeaveMulticastGroup(const VIASocketAddress6 *pAddress, uint32 ifIndex) = 0;

    VIASTDDECL SetMulticastInterface(uint32 ifIndex) = 0;

#endif
#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 6)
    // get the address family of the socket
    VIASTDDECL GetAddressFamily(VIAAddressFamily *afFamily) = 0;
    // get the local address and port, the socket is bound to
    VIASTDDECL GetSocketName(VIAEndpoint *pAddress) = 0;

#endif
#if defined ( VIAMinorVersion) && (VIAMinorVersion >= 63)
#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 7)
    // gets the remote port if the VIASocket is a tcp connection
    VIASTDDECL GetRemotePort(uint16 *pPort) = 0;

#endif
#endif

};

// ============================================================================
// class VIASocketClient
//
//                !!!! obsolete -> use VIASocketClient2 !!!!!
//
// ============================================================================
class VIASocketClient {
public:
    // 
    VIASTDDECL OnSocketSend(VIASocket *pSocket, void *pParam, int32 result, int32 protocol, int8 *pBuffer,
                            uint32 size) = 0;
    // 
    VIASTDDECL OnSocketReceive(VIASocket *pSocket, void *pParam, int32 result, int32 protocol, uint32 address,
                               uint16 port, int8 *pBuffer, uint32 size) = 0;
    // 
    VIASTDDECL OnSocketReceive(VIASocket *pSocket, void *pParam, int32 result, int32 protocol,
                               const VIASocketAddress6 *pAddress, uint16 port, int8 *pBuffer, uint32 size) = 0;
    // 
    VIASTDDECL OnSocketListen(VIASocket *pSocket, int32 result) = 0;
    // 
    VIASTDDECL OnSocketConnect(VIASocket *pSocket, int32 result) = 0;
    // 
    VIASTDDECL OnSocketClose(VIASocket *pSocket, int32 result) = 0;
};

// ============================================================================
// class VIASocketService 
// 
//               !!!! obsolete -> use VIASocketService2 !!!!!
//
// ============================================================================
class VIASocketService {
public:
    typedef enum {
        // special sinks
        kVIA_SocketRaw = 0,
        kVIA_SocketUDP,
        kVIA_SocketTCP,
    } VIASocketType;

    // 
    VIASTDDECL CreateSocket(VIASocketType type, VIASocketClient *pClient, VIASocket **ppSocket) = 0;
    // 
    VIASTDDECL ReleaseSocket(VIASocket *pSocket) = 0;
    // 
    VIASTDDECL GetAdapterCount(uint32 *pCount) = 0;
    // 
    VIASTDDECL GetAdapterAddress(uint32 ifIndex, uint32 *pAddress) = 0;
    // 
    VIASTDDECL GetAdapterAddress(uint32 ifIndex, VIASocketAddress6 *pAddress) = 0;
    // 
    VIASTDDECL GetAdapterAddressAsString(uint32 ifIndex, int8 buffer[], uint32 size) = 0;
    // 
    VIASTDDECL GetAdapterMask(uint32 ifIndex, uint32 *pAddress) = 0;
    // 
    VIASTDDECL GetAdapterMask(uint32 ifIndex, VIASocketAddress6 *pAddress) = 0;
    // 
    VIASTDDECL GetAdapterMaskAsString(uint32 ifIndex, int8 buffer[], uint32 size) = 0;
    // 
    VIASTDDECL GetAdapterGateway(uint32 ifIndex, uint32 *pAddress) = 0;
    // 
    VIASTDDECL GetAdapterGateway(uint32 ifIndex, VIASocketAddress6 *pAddress) = 0;
    // 
    VIASTDDECL GetAdapterGatewayAsString(uint32 ifIndex, int8 buffer[], uint32 size) = 0;
    // 
    VIASTDDECL GetAdapterDescription(uint32 ifIndex, int8 buffer[], uint32 size) = 0;
    // 
    VIASTDDECL GetAddressAsString(uint32 address, int8 buffer[], uint32 size) = 0;
    // 
    VIASTDDECL GetAddressAsString(const VIASocketAddress6 *pAddress, int8 buffer[], uint32 size) = 0;
    // 
    VIASTDDECL GetAddressAsNumber(int8 buffer[], uint32 *pAddress) = 0;
    // 
    VIASTDDECL GetAddressAsNumber(int8 buffer[], VIASocketAddress6 *pAddress) = 0;
    // 
    VIASTDDECL GetLastError(int32 *error) = 0;

#if defined ( VIAMinorVersion) && (VIAMinorVersion >= 45)
#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 1)
    // 
    VIASTDDECL GetVersion(int32 *major, int32 *minor) = 0;

#endif
#endif
#if defined ( VIAMinorVersion) && (VIAMinorVersion >= 49)
#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 3)
    // returns the application channel for which the adapter has been created, does not work for Windows-Stack
    VIASTDDECL GetAdapterApplicationChannel(uint32 ifIndex, VIAChannel *channel, VIABusInterfaceType *type) = 0;

#endif
#endif
#if defined ( VIAMinorVersion) && (VIAMinorVersion >= 50)
#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 3)
    // get a array of adapter IPv4 addresses from the interface which is identified by ifIndex 
    // the size of the array is given in parameter size. When the function returns the count of addresses
    // in the array is returned also in the parameter size
    VIASTDDECL GetAdapterAddresses(uint32 /*in*/ ifIndex, uint32 /*in*/ pAddress[], uint32 * /*in-out*/ size) = 0;
    // get a array of adapter IPv6 addresses from the interface which is identified by ifIndex 
    // the size of the array is given in parameter size. When the function returns the count of addresses
    // in the array is returned also in the parameter size
    VIASTDDECL GetAdapterAddresses(uint32 /*in*/ ifIndex, VIASocketAddress6 /*in*/ pAddress[],
                                   uint32 * /*in-out*/ size) = 0;
    // get a array of adapter IPv4 masks from the interface which is identified by ifIndex.
    // the size of the array is given in parameter size. When the function returns the count of addresses
    // in the array is returned also in the parameter size
    VIASTDDECL GetAdapterMasks(uint32 /*in*/ ifIndex, uint32 /*in*/ pAddress[], uint32 * /*in-out*/ size) = 0;

#endif
#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 6)
    // get the mac id of the interface with the given interface index
    VIASTDDECL GetAdapterMacId(uint32 /*in*/ ifIndex, VIASocketAddressEth * /*out*/ pAddress) = 0;
    // get a array of VIASocketAddresses with all assigned addresses from the interface with the given interface index. 
    // if the method is called with parameter size == 0 it will return the needed array size in the parameter size.
    // The returned addresses can be a MAC address, IPv4 or IPv6 address.
    VIASTDDECL GetAdapterSockAddresses(uint32 /*in*/ ifIndex, VIASocketAddress /*out*/ pAddresses[],
                                       uint32 * /*in-out*/ size) = 0;
    // get the count of addresses of the given address family (VIA_AF_INET or VIA_AF_INET6 allowed)
    VIASTDDECL GetAdapterAddressCount(uint32 /*in*/ ifIndex, VIAAddressFamily /*in*/ family,
                                      uint32 * /*out*/ addrCount) = 0;

#endif
#endif
};

#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 5)

// ============================================================================
// class VIASocketServiceEx , does not work for Windows-Stack 
// 
//           !!!! obsolete -> use VIASocketServiceEx2 !!!!!
//
// ============================================================================
class VIASocketServiceEx {
public:
    // get the last error code
    VIASTDDECL GetLastError(int32 *pError) = 0;
    // returns the application channel for which the adapter has been created
    VIASTDDECL GetAdapterApplicationChannel(uint32 ifIndex, VIAChannel *channel, VIABusInterfaceType *type) = 0;
    // add a IPv4/IPv6 address to the interface identified by ifIndex
    VIASTDDECL AddAdapterAddress(uint32 /*in*/ ifIndex, const VIANetworkAddress /*in*/ *address) = 0;
    // remove a IPv4/IPv6 address from the interface identified by ifIndex
    VIASTDDECL RemoveAdapterAddress(uint32 /*in*/ ifIndex, const VIANetworkAddress /*in*/ *address) = 0;
    // set the IPv4/IPv6 gateway address
    VIASTDDECL SetAdapterGateway(const VIASocketAddress /*in*/ *address) = 0;

    // add a static route from destination to gateway in the network stack.
    // Its possible to add a host or network route. If there is a prefix set
    // in the destination address a network route will be installed otherwise
    // a host route will be installed.
    // To add a static route to the IP routing tables the destination and
    // gateway addresses has to be from the same type of network address
    // (VIA_AF_INET or VIA_AF_INET6).
    // To install a interface route in the IP routing table the gateway address
    // has to be of type VIA_AF_LINK with a all zero ethernet address but the
    // interface index has to be set.
    // To add a static route to the ARP routing table the destination address
    // has to be of type VIA_AF_INET or VIA_AF_INET6 and the gateway address
    // has to be of type VIA_AF_LINK with a valid ethernet address (!=00:00:00
    // :00:00:00)
    // If the interface index of the gateway is set to zero, the routing command
    // try to find the correct interface automatically. Therefore the command
    // checks if there already exists a route to destination (for instance a
    // network route) and takes the same interface.
    VIASTDDECL AddRoute(const VIANetworkAddress * /*in*/ destination, const VIANetworkAddress * /*in*/ gateway) = 0;
    // change a route from destination to gateway in the network stack see AddRoute
    // for details
    VIASTDDECL ChangeRoute(const VIANetworkAddress * /*in*/ destination, const VIANetworkAddress * /*in*/ gateway) = 0;
    // delete a route to destination from the network stack
    VIASTDDECL DeleteRoute(const VIANetworkAddress * /*in*/ destination, const VIANetworkAddress * /*in*/ gateway) = 0;
    // get the vlan id and the vlan priority of the interface with the given index.
    VIASTDDECL GetAdapterVlanParameter(uint32 /*in*/ ifIndex, uint16 /*out*/ *vlanId, uint16 /*out*/ *vlanPrio) = 0;

#if defined ( VIASocketMinorVersion) && (VIASocketMinorVersion >= 8)
    // set a sysctl parameter in the tcp/ip stack by its path and name (e.g. "net.inet.tcp.delayed_ack")
    VIASTDDECL SetStackParameter(const char * /*in*/ parameterName, int32 /*in*/ value) = 0;
    // read a sysctl parameter from the tcp/ip stack by its path and name (e.g. "net.inet.tcp.delayed_ack")
    VIASTDDECL GetStackParameter(const char * /*in*/ parameterName, int32 * /*out*/ value) = 0;
    // set a parameter in the tcp/ip stack for a given interface by its path and name (e.g. "canoe.interface.mtu")
    VIASTDDECL SetInterfaceParameter(uint32 /*in*/ ifIndex, const char * /*in*/ parameterName, int32 /*in*/ value) = 0;
    // read a parameter from the tcp/ip stack for a given interfyce by its path and name (e.g. "canoe.interface.mtu")
    VIASTDDECL GetInterfaceParameter(uint32 /*in*/ ifIndex, const char * /*in*/ parameterName,
                                     int32 * /*out*/ value) = 0;

#endif
};

#endif

#endif // VIA_SOCKET_H
