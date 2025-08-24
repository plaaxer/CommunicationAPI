// address.hpp
#ifndef PROTOCOL_ADDRESS_HPP
#define PROTOCOL_ADDRESS_HPP

#include "api/network/ethernet.hpp" // For Physical_Address

// Port for now is just an int
typedef int Port;

class ProtocolAddress {
public:
    ProtocolAddress(Ethernet::MAC paddr = {}, Port port = 0)
        : _paddr(paddr), _port(port) {}

    operator bool() const {
        // for now suppose a valid ProtocolAddress has a non-zero port
        return _port != 0;
    }

    bool operator==(const ProtocolAddress& other) const {
        return (_paddr == other._paddr) && (_port == other._port);
    }

    Port port() const { return _port; }

private:
    Ethernet::MAC _paddr;
    Port _port;
};

#endif // PROTOCOL_ADDRESS_HPP