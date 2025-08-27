// address.hpp
#ifndef ADDRESS_HPP
#define ADDRESS_HPP

#include "api/network/ethernet.hpp" // For Physical_Address


class Address 
{
public:
    // Port for now is just an int
    typedef unsigned int Port;
    
public:
    
    Address(Ethernet::MAC paddr = {}, Port port = 0)
    : _paddr(paddr), _port(port) {}
    
    static const Address& broadcast() {
        static const Address b{Ethernet::BROADCAST_ADDR, 9001};
        return b;
    }

    operator bool() const {
        // foo now suppose a valid address has a non-zero port
        return _port != 0;
    }

    bool operator==(const Address& other) const {
        printf("Comparando endereços \n");
        printf("  Endereço 1: 0x%04X:%u\n", _paddr, _port);
        printf("  Endereço 2: 0x%04X:%u\n", other._paddr, other._port);
        return (_paddr == other._paddr) && (_port == other._port);
    }

    Port port() const { return _port; }

    Ethernet::MAC paddr() const { return _paddr; }

private:
    Ethernet::MAC _paddr;
    Port _port;
};

#endif // ADDRESS_HPP

