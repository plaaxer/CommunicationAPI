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

        void handle_ptp_message(const void* payload) {
            
            Header header = *reinterpret_cast<Header*>(payload);

            if (payload.size() < sizeof(Header)) {
                throw std::runtime_error("Payload received is too small to be a SyncPayload");
            }
            
            switch (header.type) {

                case SyncType::SYNC:
                    receive_sync(payload);

                case SyncType::DELAY_RESPONSE:
                    receive_delay_resp(payload);

                case SyncType::DELAY_REQUEST:
                    return; // only handled by the RSU
                
                default:
                    throw std::runtime_error("Unknown SyncType received when handling ptp message");
            }
                    
        }

    private:

        void receive_sync(const void* payload) {

            SyncPayload sync = *reinterpret_cast<const SyncPayload*>(payload.data());     
           
            if (sync.type != SyncType::SYNC) {
                throw std::runtime_error("Payload received by SlaveSynchronizer was not of type sync");
            }
           
            _t1 = sync.t1;
            _t2 = static_cast<int64_t>(Clock::getCurrentTimeMillis());
           
            _state = State::AWAITING_DELAY_RESP;
            
        }

        void send_delay_req() {
            // todo
        }

        void receive_delay_resp() {
            //todo
        }

};

#endif // SLAVE_SYNCHRONIZER_HPP