#ifndef GROUP_LEADER_HANDLER_HPP
#define GROUP_LEADER_HANDLER_HPP

#include "api/network/definitions/network_types.hpp"
#include "api/network/groups/i_group_handler.hpp"
#include "api/network/groups/group_payload.hpp"
#include "api/network/crypto/crypto_utils.hpp"
#include "api/network/definitions/address.hpp"
#include "api/utils/clock.hpp"
#include <vector>
#include <mutex>
#include <iostream>
#include <map>
#include <unordered_map>

template<typename LocalNIC, typename ExternalNIC>
class GroupLeaderHandler : public IGroupHandler<LocalNIC, ExternalNIC>
{

public:
    using Timepoint = std::chrono::time_point<std::chrono::steady_clock>;

private:
    Protocol<LocalNIC, ExternalNIC>& _protocol;
    SessionKey _session_key;

    // Group membership management
    static constexpr std::chrono::milliseconds _timeout_duration{10};

    std::thread _monitor_thread;
    std::condition_variable _cv;  // optimized monitor thread

    // O(1) access to the next to expire client timeout
    std::multimap<Timepoint, Address> _clients_timeout;
    // Lookup by ID: a optimized way to O(1) client removal
    std::unordered_map<Address, std::multimap<Timepoint, Address>::iterator> _client_lookup;

    std::mutex _mutex;
    std::atomic<bool> _running;

public:

    GroupLeaderHandler(Protocol<LocalNIC, ExternalNIC>& proto)
        : _protocol(proto)
    {
        SessionKey key = CryptoUtils::generate_session_key();

        // updates the SharedMemory SessionKey value
        _protocol.set_session_key(key);

        start_membership_monitor();

        std::cout << "[GroupLeader] Active with session key: " << key << std::endl;
    }

    ~GroupLeaderHandler() override
    {
        stop_membership_monitor();
    }

    /**
     * @brief Handles incoming group messages from the Protocol.
     */
    void handle_group_message(const void* payload, size_t size, const Address& from) override {
        const auto* header = static_cast<const GroupPayload::Header*>(payload);

        if (header->type == GroupPayload::Type::JOIN_REQUEST) {
            std::cout << "[GroupLeader] Received JOIN request from " << from << std::endl;

            heartbeat_update(from);

            send_key(from);
        }

    }

    void start_membership_monitor()
    {
        _running = true;
        _monitor_thread = std::thread(&GroupLeaderHandler::monitor_loop, this);
    }

    void stop_membership_monitor()
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _running = false;
        }

        _cv.notify_one();
        if (_monitor_thread.joinable()) {
            _monitor_thread.join();
        }
    }

    /**
     * @brief Called every time a Service message comes
     */
    void heartbeat_update(const Address& client) override
    {
        std::lock_guard<std::mutex> lock(_mutex);

        auto now = std::chrono::steady_clock::now();
        auto new_expiry = now + _timeout_duration;

        // check if client the client is already registered 
        auto it = _client_lookup.find(client);
        if (it != _client_lookup.end()) {
            // OPTIMIZATION: We have the iterator to the old timer node.
            // We erase it directly without searching the multimap.
            _clients_timeout.erase(it->second);
        } else {
            // new client detected
            std::cout << "New client joined: " << client << std::endl;
        }

        // insert new expiry time into the ordered map
        auto timer_it = _clients_timeout.emplace(new_expiry, client);

        // Update the lookup map with the new iterator
        _client_lookup[client] = timer_it;

        // notify the monitoring thread that the schedule changed
        _cv.notify_one();

    }

private:
    /**
     * @brief Sends the current session key (unicast) to a new member.
     */
    void send_key(const Address& member_address) {
        
        struct KeyPacket {
            Segment::Header seg_header;
            GroupPayload::KeyDistributionPayload key_payload;
        } __attribute__((packed));

        KeyPacket packet;

        packet.seg_header.type = Segment::MsgType::GROUP;
        // NS OR MS?
        packet.seg_header.timestamp = Clock::getCurrentTimeNanos(); // TODO: Get synchronized time
        // Gustavo: is it only this? the Leader is already the MasterSync, so the sync time is your own clock, no?

        packet.key_payload.type = GroupPayload::Type::KEY_DISTRIBUTION;
        packet.key_payload.key = _session_key;

        Address my_rsu_address = _protocol.get_external_address(); 
        _protocol.send(my_rsu_address, member_address, &packet, sizeof(packet));
        
        std::cout << "[GroupLeader] Sent session key to " << member_address << std::endl;
    }

    void monitor_loop()
    {
        std::unique_lock<std::mutex> lock(_mutex);

        while (_running) {
            // If there is no clients yet, the thread should sleep
            if (_clients_timeout.empty()) {
                // sleeps until someone joins or the thread stop
                _cv.wait(lock, [this] { return !_clients_timeout.empty() || !_running; });
            } else {
                auto earliest_expiry = _clients_timeout.begin()->first;

                if (_cv.wait_until(lock, earliest_expiry) == std::cv_status::timeout) {

                    auto now = std::chrono::steady_clock::now();

                    // find all clients that have expired your heartbeats
                    while (!_clients_timeout.empty() && _clients_timeout.begin()->first <= now) {
                        auto it = _clients_timeout.begin();
                        Address client = it->second;

                        // notify all peers in the group that one client is no longer in the membership
                        // unlocks during callback to prevent deadlocks in the API
                        lock.unlock();
                        send_notify_left(client);
                        lock.lock();

                        // clean up maps
                        _client_lookup.erase(client);
                        _clients_timeout.erase(it);
                    }
                }
            }
        }
    }

    /**
     * @brief Builds the NOTIFY_LEFT packet and send broadcasted in the quadrant
     */
    void send_notify_left(const Address& client)
    {
        struct NotifyLeftSegment {
            Segment::Header seg_header;
            NotifyLeftPayload notify_left_payload;            
        };  // __attribute__((packed));

        NotifyLeftSegment segment;

        segment.seg_header.type = Segment::MsgType::GROUP;
        segment.seg_header.timestamp = Clock::getCurrentTimeMillis();

        segment.notify_left_payload.type = GroupPayload::Type::NOTIFY_LEFT;

        _protocol.send(_protocol.get_external_address(), Address(Ethernet::BROADCAST_ADDR, 0), &segment, sizeof(segment));
    }
};

#endif // GROUP_LEADER_HANDLER_HPP