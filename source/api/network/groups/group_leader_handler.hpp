#ifndef GROUP_LEADER_HANDLER_HPP
#define GROUP_LEADER_HANDLER_HPP

#include "api/network/groups/i_group_handler.hpp"
#include "api/network/groups/group_payload.hpp"
#include "api/network/crypto/i_crypto_provider.hpp"
#include <vector>
#include <mutex>
#include <iostream>

template<typename LocalNIC, typename ExternalNIC>
class GroupLeaderHandler : public IGroupHandler<LocalNIC, ExternalNIC> {

private:
    Protocol<LocalNIC, ExternalNIC>& _protocol;
    SessionKey _session_key;
    std::vector<Address> _members;
    std::mutex _mutex; // To protect the _members list

public:
    GroupLeaderHandler(Protocol<LocalNIC, ExternalNIC>& proto)
        : _protocol(proto) {
        // TODO: Generate a real random key
        _session_key = 0xDEADBEEFCAFEBABE; 
        std::cout << "[GroupLeader] Active with session key: " << _session_key << std::endl;
    }

    ~GroupLeaderHandler() override = default;

    /**
     * @brief Handles incoming group messages from the Protocol.
     */
    void handle_group_message(const void* payload, size_t size, const Address& from) override {
        const auto* header = static_cast<const GroupPayload::Header*>(payload);

        if (header->type == GroupPayload::Type::JOIN_REQUEST) {
            std::cout << "[GroupLeader] Received JOIN request from " << from << std::endl;
            // TODO: Add logic to check if 'from' is already in _members
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _members.push_back(from);
            }
            send_key(from);
        }
        // Ignores other message types
    }

    /**
     * @brief Returns the group's session key.
     */
    SessionKey get_session_key() const override {
        return _session_key;
    }

private:
    /**
     * @brief Sends the current session key (unicast) to a new member.
     */
    void send_key(const Address& member_address) {
        // 1. Define the packet structure on the stack
        struct KeyPacket {
            Segment::Header seg_header;
            GroupPayload::KeyDistributionPayload key_payload;
        } __attribute__((packed));

        KeyPacket packet;

        // 2. Fill the Segment header
        packet.seg_header.type = Segment::MsgType::GROUP_MGMT;
        packet.seg_header.timestamp = 0; // TODO: Get synchronized time

        // 3. Fill the GroupPayload header
        packet.key_payload.type = GroupPayload::Type::KEY_DISTRIBUTION;
        packet.key_payload.key = _session_key;

        // 4. Send the packet
        // TODO: Need a way to get the RSU's own address
        Address my_rsu_address = _protocol.get_external_address(); 
        _protocol.send(my_rsu_address, member_address, &packet, sizeof(packet));
        
        std::cout << "[GroupLeader] Sent session key to " << member_address << std::endl;
    }
};

#endif // GROUP_LEADER_HANDLER_HPP