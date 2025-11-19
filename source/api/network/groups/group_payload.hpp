#ifndef GROUP_PAYLOAD_HPP
#define GROUP_PAYLOAD_HPP

#include <cstdint>
#include "api/network/definitions/segment.hpp"
#include "api/network/definitions/network_types.hpp"
#include "api/network/definitions/address.hpp"

namespace GroupPayload {
    

    // subtypes of GROUP_MGMT messages
    enum class Type : uint8_t {
        JOIN_REQUEST,      // Member -> Leader
        KEY_DISTRIBUTION,   // Leader -> Member (Unicast)
        NOTIFY_LEFT  // Leader -> Members (Broadcast)
    }; 

    struct Header {
        Type type;
    } __attribute__((packed));

    struct JoinRequestPayload : Header {
        // Timestamp ts;  // MAYBE WE CAN USE THE TIMESTAMP IN THE SEGMENT HEADER
    } __attribute__((packed));

    struct KeyDistributionPayload : Header {
        SessionKey key;
    } __attribute__((packed));

    struct NotifyLeftPayload : Header {
        Address member;
    }; // __attribute__((packed));

} // namespace GroupPayload

#endif // GROUP_PAYLOAD_HPP