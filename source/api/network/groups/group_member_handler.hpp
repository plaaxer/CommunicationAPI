#ifndef GROUP_MEMBER_HANDLER_HPP
#define GROUP_MEMBER_HANDLER_HPP

#include "api/network/groups/i_group_handler.hpp"
#include "api/network/groups/group_payload.hpp"
#include <atomic>
#include <iostream>

template<typename LocalNIC, typename ExternalNIC>
class GroupMemberHandler : public IGroupHandler<LocalNIC, ExternalNIC> {

private:
    Protocol<LocalNIC, ExternalNIC>& _protocol;
    std::atomic<SessionKey> _session_key;

public:
    GroupMemberHandler(Protocol<LocalNIC, ExternalNIC>& proto)
        : _protocol(proto), _session_key(0) {
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
            _session_key.store(key_payload->key);
            std::cout << "[GroupMember] Received and stored new session key!" << std::endl;
        }

        // todo: it should now call protocol to store the session key in the shared memory.
    }

};

#endif // GROUP_MEMBER_HANDLER_HPP