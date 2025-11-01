#ifndef TIME_PAYLOAD_HPP
#define TIME_PAYLOAD_HPP

#include <cstdint>
#include "api/network/definitions/segment.hpp"


namespace TimePayload {

     /*
     So, actually, all messages contain timestamps nonetheless, at the Segment Header. However,
     for clarity, on time synchronization messages we shall also explicitly fill the timestamps
     inside the synchronization payload.
     */

    using Timestamp = Segment::Timestamp;

    enum class SyncType : uint8_t {
        SYNC,
        DELAY_REQUEST,
        DELAY_RESPONSE,
        START
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

   // START
   struct StartPayload : Header { } __attribute__((packed));

} // namespace TimeSync

#endif // TIME_PAYLOAD_HPP