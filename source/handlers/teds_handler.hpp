#ifndef TEDS_HANDLER_HPP
#define TEDS_HANDLER_HPP

#pragma once

#include "handlers/i_handler.hpp"
#include "vm/vehicle/components/i_component_bridge.hpp"
#include "api/network/protocol.hpp"
#include "api/network/definitions/teds.hpp"
#include "api/network/definitions/address.hpp"
#include "utils/logger.cpp"

#include <map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <bitset>

using LocalProtocol = Protocol<NIC<ShmEngine>>;
template<typename LocalSmartData>

class TEDSHandler : public ITEDSHandler {
public:

    TEDSHandler(IComponentBridge<LocalSmartData>& component_bridge)
        : _component_bridge(component_bridge), _running(true)
    {
        _cleanup_thread = std::thread(&TEDSHandler::cleanup_loop, this);
    }

    ~TEDSHandler() override {
        _running = false;
        if (_cleanup_thread.joinable()) {
            _cleanup_thread.join();
        }
    }

    /**
     * @brief Handles TEDS messages in general, such as interest and response (handled by actuators and sensors, respectively).
     */
    virtual void handleTEDSMessage(Communicator<LocalProtocol>& comm, const Message& msg) override 
    {
        const std::vector<char>& payload = msg.get_payload();
        const void* raw_data = payload.data();

        const TEDS::Header* header = static_cast<const TEDS::Header*>(raw_data);
        TEDS::Type teds_type = header->type;
        const void* teds_data = static_cast<const char*>(raw_data) + sizeof(TEDS::Header);

        // SENSOR
        if (TEDS::is_request(teds_type)) {

            auto* request = static_cast<const TEDS::RequestPayload*>(teds_data);
            TEDS::Period period = request->interval_ms;

            LOG_STREAM << "[TEDS Handler] Received INTEREST (Period: " << period << "ms).";

            Address src = msg.source();
            
            std::vector<SubscriberInfo>& list = _subscribers[teds_type];

            // checking if vehicle is already subscribed (maybe with new period?)
            bool found = false;
            for (auto& sub : list) {
                if (sub.address == src) {
                    sub.period = period;
                    found = true;
                    break;
                }
            }

            if (!found) {
                list.push_back({src, period});
            }

             _component_bridge.notify_interest_request(period, teds_type);

        // ACTUATOR
        } else if (TEDS::is_response(teds_type)) {

            LOG_STREAM << "[TEDS Handler] Received RESPONSE for TEDS type: " << TEDS::get_type_name(teds_type);
            
            _component_bridge.apply_value_from_payload(msg.get_payload());
            
        } else {
            std::cout << "[TEDS Handler] Received unknown TEDS message type." << std::endl;
        }
    }

    /**
     * @brief Reactively sends a TEDS RESPONSE message
     */
    void send_response(Communicator<LocalProtocol>& comm, Address dest, TEDS::Type type) override
    {

        float sensor_data = _component_bridge.get_value();

        LOG_STREAM << "[TEDS Handler] Sending RESPONSE to " << dest.paddr();

        if (sensor_data) {

            Message response_msg(dest, type, sensor_data);
            response_msg.set_source(comm.address());
            
            comm.send(&response_msg);

        }
    }

    /**
     * @brief Helper debugging method (TEDS Type)
     */
    template<typename T>
    void print_bits(const T& value, const std::string& label = "") {

        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&value);
        
        if (!label.empty()) {
            std::cout << label << " ";
        }

        for (size_t i = 0; i < sizeof(T); ++i) {
            std::cout << std::bitset<8>(bytes[i]) << " ";
        }
        std::cout << std::endl;
    }

private:

    /**
     * @brief Every x seconds performs cleanups; checks if subscribed/interested vehicles are still in range.
     */
    void cleanup_loop() {

        while (_running) {

            std::this_thread::sleep_for(std::chrono::seconds(3));
            
            std::lock_guard<std::mutex> lock(_subscribers_mutex);

            for (auto it = _subscribers.begin(); it != _subscribers.end(); ) {
                TEDS::Type type = it->first;
                std::vector<SubscriberInfo>& list = it->second;
                bool list_changed = false;

                auto new_end = std::remove_if(list.begin(), list.end(), 
                    [this](const SubscriberInfo& sub) {
                        if (!_component_bridge.is_entity_nearby(sub.address)) {
                            std::cout << "[TEDS Handler] Removing distant subscriber: " << sub.address.paddr() << std::endl;
                            return true;
                        }
                        return false;
                    });

                if (new_end != list.end()) {
                    list.erase(new_end, list.end());
                    list_changed = true;
                }

                if (list_changed) {
                    if (list.empty()) {
                        _component_bridge.notify_interest_request(0, type, true); 
                    } else {
                        TEDS::Period new_gcd = list[0].period;
                        for (size_t i = 1; i < list.size(); ++i) {
                            new_gcd = std::gcd(new_gcd, list[i].period);
                        }
                        _component_bridge.notify_interest_request(new_gcd, type, true);
                    }
                }
                
                if (list.empty()) {
                     it = _subscribers.erase(it);
                } else {
                     ++it;
                }
            }
        }
    }

    struct SubscriberInfo {
        Address address;
        TEDS::Period period;
    };

    IComponentBridge<LocalSmartData>& _component_bridge;

    std::map<TEDS::Type, std::vector<SubscriberInfo>> _subscribers;

    std::mutex _subscribers_mutex;
    std::thread _cleanup_thread;
    std::atomic<bool> _running;
    
};

#endif  // TEDS_HANDLER.HPP