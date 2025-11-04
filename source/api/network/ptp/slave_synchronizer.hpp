#ifndef SLAVE_SYNCHRONIZER_HPP
#define SLAVE_SYNCHRONIZER_HPP

#include "api/network/definitions/segment.hpp"
#include "api/network/definitions/time_payload.hpp" 
#include "api/utils/clock.hpp"
#include "api/network/ptp/i_synchronizer.hpp"

template<typename LocalNIC, typename ExternalNIC> class Protocol; 

// see https://networklessons.com/ip-services/introduction-to-precision-time-protocol-ptp

using namespace TimePayload;

template<typename LocalNIC, typename ExternalNIC>
class SlaveSynchronizer : public ISynchronizer {
    public:
        using Timestamp = Segment::Timestamp;

    private:
        Timestamp _t1 = 0; // master send sync
        Timestamp _t2 = 0; // slave receive sync
        Timestamp _t3 = 0; // slave send delay req
        Timestamp _t4 = 0; // master receive delay req

        Protocol<LocalNIC, ExternalNIC>& _protocol;

        enum class State { SYNCHRONIZED, AWAITING_SYNC, AWAITING_DELAY_RESP };
        std::atomic<State> _state;

        
    public:

        SlaveSynchronizer(Protocol<LocalNIC, ExternalNIC>& prot) : _protocol(prot), _state(State::SYNCHRONIZED) {}

    /**
     * @brief Main dispatcher for incoming PTP messages.
     * @param payload A pointer to the raw PTP payload (starts with TimeSync::Header).
     * @param size The total size of the payload in bytes.
     */
    void handle_ptp_message(const void* payload, size_t size, const Address& source_address, const Address& dest_address) override
    {
        
        if (size < sizeof(TimePayload::Header)) {
            throw std::runtime_error("PTP payload is too small to contain a header.");
        }

        const TimePayload::Header* header = static_cast<const TimePayload::Header*>(payload);

        switch (header->type) {

            case TimePayload::SyncType::SYNC:

                if (size < sizeof(TimePayload::SyncPayload)) {
                    throw std::runtime_error("PTP SYNC payload size mismatch.");
                }

                receive_sync(static_cast<const TimePayload::SyncPayload*>(payload), source_address, dest_address);
                break;

            case TimePayload::SyncType::DELAY_RESPONSE:

                if (size < sizeof(TimePayload::DelayRespPayload)) {
                    throw std::runtime_error("PTP DELAY_RESP payload size mismatch.");
                }

                receive_delay_resp(static_cast<const TimePayload::DelayRespPayload*>(payload));
                break;

            case TimePayload::SyncType::DELAY_REQUEST:
                return;
            
            case TimePayload::SyncType::START:  // slaves also do not deal with start
                return;
            
            default:
                throw std::runtime_error("Unknown SyncType received when handling PTP message");
        }
    }

    /**
     * @brief Kicks off the PTP synchronization process.
     * This is called by the Gateway's timer thread every X seconds.
     */
    void request_synchronization(const Address& my_address) override
    {

        if (_state == State::SYNCHRONIZED) {
            
            _state = State::AWAITING_SYNC;

            struct StartPacket {
                Segment::Header seg_header;
                TimePayload::StartPayload start_payload;
            } __attribute__((packed));
            
            StartPacket packet;

            packet.seg_header.type = Segment::MsgType::PTP;
            packet.seg_header.timestamp = Clock::getCurrentTimeMillis();

            packet.start_payload.type = SyncType::START;
            
            // todo: how to get the master address? for now let's just broadcast and it should work, the RSU will filter it out.
            // but maybe the gateway should hold a value referring to its current RSU.
            _protocol.send(my_address, Address(Ethernet::BROADCAST_ADDR, 0), &packet, sizeof(packet));

            return;
        }

        std::cout << "Warning: synchronization requested with slave in invalid state" << std::endl;
    }

    private:

        void receive_sync(const TimePayload::SyncPayload* sync, const Address& source_address, const Address& dest_address)
        {

            if (_state != State::AWAITING_SYNC) {
                throw std::runtime_error("Slave was not waiting for SYNC");
            }
            
            // std::cout << "[PTP DEBUG] t1 (from packet):   " << sync->t1 << std::endl;

            _t1 = sync->t1;
            _t2 = static_cast<Timestamp>(Clock::getCurrentTimeMillis());
        
            // std::cout << "[PTP DEBUG] t2 (local clock):   " << _t2 << std::endl;
        
            _state = State::AWAITING_DELAY_RESP;
            
            // the source address of the SYNC is the master address
            send_delay_req(dest_address, source_address);
        }

        void send_delay_req(const Address& my_address, const Address& master_address)
        {
    
            _t3 = static_cast<Timestamp>(Clock::getCurrentTimeMillis());

            struct DelayReqPacket {
                Segment::Header seg_header;
                TimePayload::DelayReqPayload ptp_payload;
            } __attribute__((packed));

            DelayReqPacket packet;

            packet.seg_header.type = Segment::MsgType::PTP;
            packet.seg_header.timestamp = _t3;

            packet.ptp_payload.type = TimePayload::SyncType::DELAY_REQUEST;
            
            _protocol.send(
                my_address,
                master_address,
                &packet,
                sizeof(packet)
            );
        }

        void receive_delay_resp(const TimePayload::DelayRespPayload* resp)
        {
            
            if (_state != State::AWAITING_DELAY_RESP) {
                throw std::runtime_error("Slave was not waiting for DELAY_RESP");
            }
            
            _t4 = static_cast<Timestamp>(resp->t4); 
            
            synchronize();

            _state = State::SYNCHRONIZED;

        }

        void synchronize()
        {
            std::cout << "[PTP DEBUG] t1 (Master Sync Sent):     " << _t1 << std::endl;
            std::cout << "[PTP DEBUG] t2 (Slave Sync Rcvd):      " << _t2 << std::endl;
            std::cout << "[PTP DEBUG] t3 (Slave DelayReq Sent):  " << _t3 << std::endl;
            std::cout << "[PTP DEBUG] t4 (Master DelayReq Rcvd): " << _t4 << std::endl;

            int64_t t1 = static_cast<int64_t>(_t1);
            int64_t t2 = static_cast<int64_t>(_t2);
            int64_t t3 = static_cast<int64_t>(_t3);
            int64_t t4 = static_cast<int64_t>(_t4);

            int64_t offset = ((t2 - t1) - (t4 - t3)) / 2;
    
            _t1 = 0; _t2 = 0; _t3 = 0; _t4 = 0;

            bool adjusted = Clock::setClockOffset(offset);
            
            if (!adjusted) {
                throw std::runtime_error("Failed to adjust clock skew");
            }
            // std::cout << "[PTP] Clock synchronized. Offset: " << offset << " ms." << std::endl;
            std::cout << "[PTP] Adjusted system clock: " << Clock::getCurrentTimeString() << std::endl;

        }

};

#endif // SLAVE_SYNCHRONIZER_HPP