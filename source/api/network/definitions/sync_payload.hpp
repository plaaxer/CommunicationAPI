#ifndef SYNC_PAYLOAD_HPP
#define SYNC_PAYLOAD_HPP

#include <cstdint> // For uint8_t
#include "api/network/definitions/segment.hpp"


namespace TimeSync {

    using Timestamp = Segment::Timestamp;

    enum class SyncType : uint8_t {
        SYNC,
        DELAY_REQUEST,
        DELAY_RESPONSE
    };

   struct Header {
        SyncType type;
   } __attribute__((packed));

   // SYNC
   struct SyncPayload : Header {
        Timestamp t1;
   } __attribute__((packed));

   // DELAY_RESPONSE
   struct DelayRespPayload : Header {
        Timestamp t4;
   } __attribute__((packed));

   // DELAY_REQUEST
   struct DelayReqPayload : Header { } __attribute__((packed));

} // namespace TimeSync

#endif // SYNC_PAYLOAD_HPP