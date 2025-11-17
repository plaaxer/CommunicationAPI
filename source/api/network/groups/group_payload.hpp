#ifndef GROUP_PAYLOAD_HPP
#define GROUP_PAYLOAD_HPP

#include <cstdint>
#include "api/network/definitions/segment.hpp"

namespace GroupPayload {
    

    // subtypes of GROUP_MGMT messages
    enum class Type : uint8_t {
        JOIN_REQUEST,      // Member -> Leader
        KEY_DISTRIBUTION   // Leader -> Member (Unicast)
    }; 

    struct Header {
        Type type;
    } __attribute__((packed));

    struct JoinRequestPayload : Header {
    } __attribute__((packed));

    struct KeyDistributionPayload : Header {
        Segment::Timestamp key;
    } __attribute__((packed));

} // namespace GroupPayload

#endif // GROUP_PAYLOAD_HPP