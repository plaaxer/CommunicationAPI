#ifndef GROUP_MEMBER_HANDLER_HPP
#define GROUP_MEMBER_HANDLER_HPP

#include "api/network/groups/i_group_handler.hpp"
#include "api/network/groups/group_payload.hpp"
#include "api/network/definitions/segment.hpp"
#include <atomic>
#include <iostream>

using namespace GroupPayload;

template<typename LocalNIC, typename ExternalNIC>
class GroupMemberHandler : public IGroupHandler<LocalNIC, ExternalNIC> {

private:
    Protocol<LocalNIC, ExternalNIC>& _protocol;
    // std::atomic<SessionKey> _session_key;

public:
    GroupMemberHandler(Protocol<LocalNIC, ExternalNIC>& proto)
        : _protocol(proto) {
    }

    ~GroupMemberHandler() override = default;

    /**
     * @brief Handles incoming group messages from the Protocol.
     * @details The incoming payload should contain only the segment payload.
     * Group join requests are ignored by member handlers.
     */
    void handle_group_message(const void* payload, size_t size, const Address& from) override {
        const auto* header = static_cast<const GroupPayload::Header*>(payload);

        if (header->type == GroupPayload::Type::KEY_DISTRIBUTION) {
            if (size < sizeof(GroupPayload::KeyDistributionPayload)) {
                std::cerr << "[GroupMember] Received key packet is too small!" << std::endl;
                return;
            }
            
            const auto* key_payload = static_cast<const GroupPayload::KeyDistributionPayload*>(payload);
            // _session_key.store(key_payload->key);
            _protocol.set_session_key(key_payload->key);
            std::cout << "[GroupMember] Received and stored new session key!" << std::endl;

        } else if (header->type == GroupPayload::Type::NOTIFY_LEFT) {
            if (size < sizeof(GroupPayload::NotifyLeftPayload)) {
                std::cerr << "[GroupMember] Received NotifyLeft packet is too small!" << std::endl;
                return;
            }

            const auto* notify_payload = static_cast<const GroupPayload::NotifyLeftPayload*>(payload);
            
            std::cerr << "[GroupMember] Received NotifyLeft packet: Client" << notify_payload->member
            << " has left the quadrant!" <<  std::endl;
        }
    }

    /**
     * @brief Notifies the group handler that a location change (quadrant) has ocurred.
     * Prompts it to a send a join message to the new rsu (only the rsu from the sam quadrant will receive the message, even though it is broadcast).
     * Wherever this is called a time synchronization should also be started.
     */

    void notify_location_change() override {

        struct JoinSegment {
            Segment::Header seg_header;
            JoinRequestPayload join_payload;            
        } __attribute__((packed));

        JoinSegment segment;

        segment.seg_header.type = Segment::MsgType::GROUP;
        segment.seg_header.timestamp = Clock::getCurrentTimeMillis();

        segment.join_payload.type = Type::JOIN_REQUEST;

        _protocol.send(_protocol.get_external_address(), Address(Ethernet::BROADCAST_ADDR, 0), &segment, sizeof(segment));
    }

};

#endif // GROUP_MEMBER_HANDLER_HPP