#ifndef MASTER_SYNCHRONIZER_HPP
#define MASTER_SYNCHRONIZER_HPP

#include "api/network/ptp/i_synchronizer.hpp"
#include "api/network/definitions/segment.hpp"
#include "api/network/definitions/time_payload.hpp" 
#include "api/utils/clock.hpp"

// to avoid circular dependency
template<typename LocalNIC, typename ExternalNIC> class Protocol;

template<typename LocalNIC, typename ExternalNIC>
class MasterSynchronizer : public ISynchronizer
{
public:
    
    using Timestamp = Segment::Timestamp;

    MasterSynchronizer(Protocol<LocalNIC, ExternalNIC>& prot)
        : _protocol(prot) {}
    
    ~MasterSynchronizer() override = default;
    

    void handle_ptp_message(const void* payload, size_t size, const Address& source_address, const Address& dest_address) override
    {
    
         if (size < sizeof(TimePayload::Header)) {
            throw std::runtime_error("PTP payload is too small to contain a header.");
        }

        const TimePayload::Header* header = static_cast<const TimePayload::Header*>(payload);

        switch (header->type) {

            case TimePayload::SyncType::START:
                // we should send the sync with the t1 timestamp
                
                if (size < sizeof(TimePayload::StartPayload)) {
                    throw std::runtime_error("PTP Start payload size mismatch.");
                }

                receive_start(static_cast<const TimePayload::StartPayload*>(payload), source_address, dest_address);
                break;

            case TimePayload::SyncType::DELAY_REQUEST:

                if (size < sizeof(TimePayload::DelayReqPayload)) {
                    throw std::runtime_error("PTP DELAY_REQ payload size mismatch.");
                }

                // for now we will not use the dest_address received, cause the Start is currently broadcast
                receive_delay_req(static_cast<const TimePayload::DelayReqPayload*>(payload),
                    source_address, dest_address);

                break;

            case TimePayload::SyncType::DELAY_RESPONSE:
                // in the future it is possible to a master also be a slave
                return;
            
            case TimePayload::SyncType::SYNC:
                // in the future it is possible to a master also be a slave
                return;

            default:
                throw std::runtime_error("Unknown SyncType received when handling PTP message");
        }       
    }
    
    void request_synchronization(const Address& my_address) override {
        // (empty for masters)
    }

private:

    /**
     * @brief Builds and send a Sync w/ timestamp (t1)
     */
    void receive_start(const TimePayload::StartPayload* start, const Address& source_address, const Address& dest_address)
    {
        struct SyncPacket {
            Segment::Header seg_header;
            TimePayload::SyncPayload sync_payload;
        } __attribute__((packed));

        SyncPacket packet;

        packet.seg_header.type = Segment::MsgType::PTP;
        packet.seg_header.timestamp = Clock::getCurrentTimeMillis();

        packet.sync_payload.type = TimePayload::SyncType::SYNC;

        _protocol.send(_protocol.get_external_address(), source_address, &packet, sizeof(packet));
        
    }

    /**
     * @brief Builds and send a Delay Response w/ timestamp (t3)
     */
    void receive_delay_req(const TimePayload::DelayReqPayload* delay_req, const Address& source_address, const Address& dest_address)
    {
        struct DelayRespPacket {
            Segment::Header seg_header;
            TimePayload::DelayRespPayload response_payload;
        } __attribute__((packed));
    
        DelayRespPacket packet;
    
        packet.seg_header.type = Segment::MsgType::PTP;
        packet.seg_header.timestamp = Clock::getCurrentTimeMillis();

        packet.response_payload.type = TimePayload::SyncType::DELAY_RESPONSE;

        _protocol.send(_protocol.get_external_address(), source_address, &packet, sizeof(packet));
    }

private:
    Protocol<LocalNIC, ExternalNIC>& _protocol;

};

#endif  // MASTER_SYNCHRONIZER_HPP