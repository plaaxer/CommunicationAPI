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
#include <algorithm> // For std::find
#include <iostream>
#include <bitset>

using LocalProtocol = Protocol<NIC<ShmEngine>>;
template<typename LocalSmartData>
class TEDSHandler : public ITEDSHandler {
public:
    TEDSHandler(IComponentBridge<LocalSmartData>& component_bridge)
        : _component_bridge(component_bridge)
    {}

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
            
            std::vector<Address>& list = _subscribers[teds_type];

            bool already_subscribed = false;
            for (const auto& addr : list) {
                 if (addr == src) {
                     already_subscribed = true;
                     break;
                 }
            }

            if (!already_subscribed) {
                 list.push_back(src);
                 std::cout << "Added " << src << " to subscribers of " << TEDS::get_type_name(teds_type) << std::endl;
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
        // Ask the bridge to get sensor data and package it into a payload
        // TO ADJUST THE VALUE. THE ARGUMENT type CAN TELL US WHAT PROGRAMMING TYPE THIS SHOULD BE
        float sensor_data = _component_bridge.get_value();

        LOG_STREAM << "[TEDS Handler] Sending RESPONSE to " << dest.paddr();

        if (sensor_data) {

            // the response payload is already being well formed in the Message constructor
            Message response_msg(dest, type, sensor_data);
            response_msg.set_source(comm.address());
            
            comm.send(&response_msg);

        }
    }

    template<typename T>
    void print_bits(const T& value, const std::string& label = "") {
        // Get the raw memory of the value
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&value);
        
        if (!label.empty()) {
            std::cout << label << " ";
        }

        // Print the bits for each byte in order
        for (size_t i = 0; i < sizeof(T); ++i) {
            std::cout << std::bitset<8>(bytes[i]) << " ";
        }
        std::cout << std::endl;
    }

private:

    IComponentBridge<LocalSmartData>& _component_bridge;

    std::map<TEDS::Type, std::vector<Address>> _subscribers;
    
};

#endif  // TEDS_HANDLER.HPP