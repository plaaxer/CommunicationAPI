#ifndef SLAVE_SYNCHRONIZER_HPP
#define SLAVE_SYNCHRONIZER_HPP

#include "api/network/definitions/segment.hpp"
#include "api/network/definitions/time_payload.hpp" 
#include "api/utils/clock.hpp"

template<typename LocalNIC, typename ExternalNIC> class Protocol; 

// see https://networklessons.com/ip-services/introduction-to-precision-time-protocol-ptp

using namespace TimePayload;

template<typename LocalNIC, typename ExternalNIC>
class SlaveSynchronizer {
    public:
        using Timestamp = Segment::Timestamp;

    private:
        Timestamp _t1 = 0; // master send sync
        Timestamp _t2 = 0; // slave receive sync
        Timestamp _t3 = 0; // slave send delay req
        Timestamp _t4 = 0; // master receive delay req

        enum class State{ AWAITING_SYNC, AWAITING_DELAY_RESP };

        State _state = State::AWAITING_SYNC;

        Protocol<LocalNIC, ExternalNIC>& _protocol;
        
    public:

        SlaveSynchronizer(Protocol<LocalNIC, ExternalNIC>& prot) : _protocol(prot) {}

        /**
     * @brief Main dispatcher for incoming PTP messages.
     * @param payload A pointer to the raw PTP payload (starts with TimeSync::Header).
     * @param size The total size of the payload in bytes.
     */
    void handle_ptp_message(const void* payload, size_t size, const Address& source_address) {
        
        if (size < sizeof(TimeSync::Header)) {
            throw std::runtime_error("PTP payload is too small to contain a header.");
        }

        const TimeSync::Header* header = static_cast<const TimeSync::Header*>(payload);

        switch (header->type) {

            case TimeSync::SyncType::SYNC:

                if (size < sizeof(TimeSync::SyncPayload)) {
                    throw std::runtime_error("PTP SYNC payload size mismatch.");
                }

                receive_sync(static_cast<const TimeSync::SyncPayload*>(payload), source_address);
                break;

            case TimeSync::SyncType::DELAY_RESPONSE:

                if (size < sizeof(TimeSync::DelayRespPayload)) {
                    throw std::runtime_error("PTP DELAY_RESP payload size mismatch.");
                }

                receive_delay_resp(static_cast<const TimeSync::DelayRespPayload*>(payload));
                break;

            case TimeSync::SyncType::DELAY_REQUEST:
                return;
            
            default:
                throw std::runtime_error("Unknown SyncType received when handling PTP message");
        }
    }

    private:

        /**
         * @brief Handles a fully parsed Sync message.
         */
        void receive_sync(const TimeSync::SyncPayload* sync, const Address& source_address) {

            if (_state != State::AWAITING_SYNC) {
                throw std::runtime_error("Slave was not waiting for SYNC, but rather DELAY_RESP");
            }
            
            _t1 = sync->t1;
            _t2 = static_cast<Timestamp>(Clock::getCurrentTimeMillis());
        
            _state = State::AWAITING_DELAY_RESP;
            
            send_delay_req(source_address);
        }

        void send_delay_req() {
            
            TimePayload time_payload;
            Header* header = static_cast<Header*>(time_payload);
            header->type = SyncType::DELAY_REQUEST;
            
            const char* payload = reinterpret_cast<char*>(header);

            Segment segment = Segment::from_bytes()

        }

        void receive_delay_resp() {
            //todo
        }

};

#endif // SLAVE_SYNCHRONIZER_HPP