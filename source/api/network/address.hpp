// address.hpp
#ifndef ADDRESS_HPP
#define ADDRESS_HPP

#include "api/network/ethernet.hpp" // For Physical_Address

// Port for now is just an int
typedef int Port;

class Address {
public:
    Address(Ethernet::MAC paddr = {}, Port port = 0)
        : _paddr(paddr), _port(port) {}

    operator bool() const {
        // for now suppose a valid Address has a non-zero port
        return _port != 0;
    }

    bool operator==(const Address& other) const {
        return (_paddr == other._paddr) && (_port == other._port);
    }

    Port port() const { return _port; }

    Ethernet::MAC paddr() const { return _paddr; }

private:
    Ethernet::MAC _paddr;
    Port _port;
};

#endif // ADDRESS_HPP