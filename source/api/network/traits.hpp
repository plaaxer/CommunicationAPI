// traits.hpp
#ifndef TRAITS_HPP
#define TRAITS_HPP

#include "ethernet.hpp"

// forward declaration as it is a circular dependency

template<typename NIC>
class Protocol;

template<typename T>
struct Traits {};

// A specialized Traits struct for your Protocol class
template<typename NIC>
struct Traits<Protocol<NIC>> {
    // This is where you define your custom EtherType
    static const typename Ethernet::Protocol ETHERNET_PROTOCOL_NUMBER = 0x88B5;
};

#endif // TRAITS_HPP