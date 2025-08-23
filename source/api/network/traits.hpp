// traits.hpp
#ifndef TRAITS_HPP
#define TRAITS_HPP

#include "ethernet.hpp"

// forward declaration as it is a circular dependency
// specific to each protocol

template<typename NIC>
class Protocol;

template<typename T>
struct Traits {};

template<typename NIC>
struct Traits<Protocol<NIC>> {
    static const typename Ethernet::Protocol ETHERNET_PROTOCOL_NUMBER = 0x88B5;
};

#endif // TRAITS_HPP