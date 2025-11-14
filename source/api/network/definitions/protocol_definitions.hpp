#ifndef PROTOCOL_DEFINITIONS_HPP
#define PROTOCOL_DEFINITIONS_HPP

#include "api/network/definitions/address.hpp"
#include "api/network/definitions/ethernet.hpp"

// Protocol Header (named as PortHeader, it only contains ports)
class PortHeader
{
public:
    PortHeader(Address::Port sport = 0, Address::Port dport = 0) : _sport(sport), _dport(dport) {}

    // source port and destination port
    Address::Port sport() const { return _sport; }
    Address::Port dport() const { return _dport; }

private:
    Address::Port _sport;
    Address::Port _dport;
} __attribute__((packed));


// max packet size. Nic MTU size minus header size
static const unsigned int MTU = Ethernet::MTU - sizeof(PortHeader);
typedef unsigned char Data[MTU]; // represents a byte array

// PACKET
class Packet : public PortHeader
{
public:
    Packet();
    PortHeader * portheader() { return this; }
    template<typename T>
    T * data() { return reinterpret_cast<T *>(&_data); }
private:
    Data _data;
} __attribute__((packed)); // removing padding


#endif // PROTOCOL_DEFINITIONS_HPP