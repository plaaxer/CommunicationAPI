#ifndef CLOCK_SYNCHRONIZER_HPP
#define CLOCK_SYNCHRONIZER_HPP

#include "api/network/definitions/segment.hpp"

template<typename LocalNIC, typename ExternalNIC> class Protocol; 

// see https://networklessons.com/ip-services/introduction-to-precision-time-protocol-ptp

template<typename LocalNIC, typename ExternalNIC>
class ClockSynchronizer {
    public:
        using Timestamp = Segment::Timestamp;

    private:
        Timestamp _t1 = 0; // master send sync
        Timestamp _t2 = 0; // slave receive sync
        Timestamp _t3 = 0; // slave send delay req
        Timestamp _t4 = 0; // master receive delay req

        enum class State{ AWAITING_SYNC, AWAITING_DELAY_RESP };

        State _state = State::AWAITING_SYNC;

        Protocol<LocalNIC, ExternalNIC>& _protocol_instance;

};

#endif // CLOCK_SYNCHRONIZER_HPP