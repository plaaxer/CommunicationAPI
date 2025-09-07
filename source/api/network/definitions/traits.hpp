// traits.hpp
#ifndef TRAITS_HPP
#define TRAITS_HPP

#include "api/network/definitions/ethernet.hpp"

// forward declaration as it is a circular dependency
// specific to each protocol

template<typename NIC, typename NIC2>
class Protocol;

template<typename T>
struct Traits {};

template<typename NIC, typename NIC2>
struct Traits<Protocol<NIC, NIC2>> {
    static const typename Ethernet::Protocol ETHERNET_PROTOCOL_NUMBER = 0x88B5;
};

#endif // TRAITS_HPP