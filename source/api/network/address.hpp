// address.hpp
#ifndef ADDRESS_HPP
#define ADDRESS_HPP

#include "ethernet.hpp" // For Physical_Address

// Port for now is just an int
typedef int Port;

class ProtocolAddress {
public:
    ProtocolAddress(Ethernet::Address paddr = {}, Port port = 0)
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
    Ethernet::Address _paddr;
    Port _port;
};

#endif // ADDRESS_HPP