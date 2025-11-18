#ifndef PROTOCOL_DEFINITIONS_HPP
#define PROTOCOL_DEFINITIONS_HPP

#include "api/network/definitions/address.hpp"
#include "api/network/definitions/ethernet.hpp"
#include "api/network/definitions/quadrant.hpp"
#include "api/network/crypto/i_crypto_provider.hpp"
#include <cstdint>

// --- Location Header ---
class LocationHeader
{
public:

    LocationHeader(Quadrant loc = Quadrant::SOUTH) : _location(loc) {}

    Quadrant location() const { return _location; }

protected:

    Quadrant _location;
} __attribute__((packed));


// --- Port Header ---
class PortHeader
{
public:
    PortHeader(Address::Port sport = 0, Address::Port dport = 0) : _sport(sport), _dport(dport) {}

    Address::Port sport() const { return _sport; }
    Address::Port dport() const { return _dport; }

private:
    Address::Port _sport;
    Address::Port _dport;
} __attribute__((packed));


// --- Packet Definition ---

// The total size of the combined packet header
static const unsigned int PACKET_HEADER_SIZE = sizeof(LocationHeader) + sizeof(PortHeader);

// max packet size. Nic MTU size minus header minus mac
static const unsigned int MTU = Ethernet::MTU - PACKET_HEADER_SIZE - sizeof(MsgAuthCode);
typedef unsigned char Data[MTU]; // represents a byte array

/**
 * @class Packet
 * @brief Contains both Location and Port information in its header section
 * by inheriting from both structs.
 */
class Packet : public LocationHeader, public PortHeader
{
public:
    // Default constructor, initializes location to south
    Packet() : LocationHeader(Quadrant::SOUTH), PortHeader(0, 0) {}

    // Full constructor, takes Quadrant and mac as parameters
    Packet(Quadrant loc, Address::Port sport, Address::Port dport)
        : LocationHeader(loc), PortHeader(sport, dport) {}

    PortHeader* portheader() { return this; }
    LocationHeader* locationheader() { return this; }

    // Accessor for the data payload (entire Segment)
    template<typename T>
    T * data() { return reinterpret_cast<T *>(&_data); }

private:

    Data _data;

} __attribute__((packed)); // removing padding


#endif // PROTOCOL_DEFINITIONS_HPP