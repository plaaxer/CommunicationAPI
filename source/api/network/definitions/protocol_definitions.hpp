#ifndef PROTOCOL_DEFINITIONS_HPP
#define PROTOCOL_DEFINITIONS_HPP

#include "api/network/definitions/address.hpp"
#include "api/network/definitions/ethernet.hpp"
#include <cstdint>
#include "api/network/definitions/quadrant.hpp"

// --- Location Header ---
class LocationHeader
{
public:

    LocationHeader(Quadrant loc = Quadrant::SOUTH) : _location_id(loc) {}

    Quadrant location_id() const { return _location_id; }

protected:
    // Changed member type to Quadrant
    Quadrant _location_id;
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

// max packet size. Nic MTU size minus new header size
static const unsigned int MTU = Ethernet::MTU - PACKET_HEADER_SIZE;
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

    // Full constructor, takes Quadrant as parameter
    Packet(Quadrant loc, Address::Port sport, Address::Port dport)
        : LocationHeader(loc), PortHeader(sport, dport) {}

    // Accessor methods
    PortHeader* portheader() { return this; }
    LocationHeader* locationheader() { return this; }

    // Accessor for the data payload (Segment)
    template<typename T>
    T * data() { return reinterpret_cast<T *>(&_data); }
private:
    Data _data;
} __attribute__((packed)); // removing padding


#endif // PROTOCOL_DEFINITIONS_HPP