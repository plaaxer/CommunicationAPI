#ifndef ETHERNET_HPP
#define ETHERNET_HPP

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cstring>

// pragma push tells the compiler to pack struct members tightly,
// without any padding. Apparently, essential for network protocols
#pragma pack(push, 1)

class Ethernet 
{
public:
    static const unsigned int ADDR_LEN = 6;    // length of a MAC Address in bytes
    inline static const uint8_t BROADCAST_ADDR[ADDR_LEN]{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    static const unsigned int MTU = 1500;      // Maximum Transmission Unit

    typedef uint16_t Protocol;

    /**
     * @struct Address
     * @brief Represents a 6-byte Ethernet MAC address.
     */
    struct MAC {
        uint8_t addr[ADDR_LEN];

        MAC() {
            memset(addr, 0, ADDR_LEN);
        }

        MAC(const uint8_t* a) {
            memcpy(addr, a, ADDR_LEN);
        }

        // Comparison operator
        bool operator==(const MAC& other) const {
            return memcmp(addr, other.addr, ADDR_LEN) == 0;
        }

    };

    /**
     * @struct Header
     * @brief Represents the 14-byte Ethernet header.
     */
    struct Header {
        MAC dhost; // Destination MAC address (6 bytes)
        MAC shost; // Source MAC address (6 bytes)
        Protocol type; // EtherType/Protocol (2 bytes)
    };

    /**
     * @struct Frame
     * @brief Represents a complete Ethernet frame, including header and data.
     */
    struct Frame {
        Header header;
        uint8_t data[MTU];
        unsigned int data_length; // includes Protocol Header (source/dest ports)
    };

};

// restore previous packing setting
#pragma pack(pop)


// --- UTILITY ---

/**
 * @brief Overloads the << operator to allow easy printing of MAC addresses.
 * Ex: std::cout << my_mac_address;
 */
inline std::ostream& operator<<(std::ostream& os, const Ethernet::MAC& addr) {
    os << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < Ethernet::ADDR_LEN; ++i) {
        os << std::setw(2) << static_cast<int>(addr.addr[i]);
        if (i < Ethernet::ADDR_LEN - 1) {
            os << ":";
        }
    }
    return os << std::dec; // Reset to decimal for subsequent outputs
}


#endif // ETHERNET_HPP
