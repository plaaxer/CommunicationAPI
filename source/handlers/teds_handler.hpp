#ifndef TEDS_HANDLER_HPP
#define TEDS_HANDLER_HPP

#pragma once

#include "handlers/i_handler.hpp"
#include "vehicle/components/i_component_bridge.hpp"
#include "api/network/protocol.hpp"
#include "api/network/definitions/teds.hpp"

#include <iostream>
#include <bitset>

using LocalProtocol = Protocol<NIC<ShmEngine>>;
template<typename LocalSmartData>
class TEDSHandler : public ITEDSHandler {
public:
    TEDSHandler(IComponentBridge<LocalSmartData>& component_bridge)
        : _component_bridge(component_bridge)
    {}

    virtual void handleTEDSMessage(Communicator<LocalProtocol>& comm, const Message& msg) override 
    {
        const std::vector<char>& payload = msg.get_payload();
        const void* raw_data = payload.data();

        // // --- DEBUG PRINTING ---
        // std::cout << "\n--- TEDS PAYLOAD DEBUG ---" << std::endl;
        // std::cout << "Total Payload Size: " << payload.size() << " bytes" << std::endl;
        // print_bits(*static_cast<const uint64_t*>(raw_data), "First 8 bytes (raw):");

        const TEDS::Header* header = static_cast<const TEDS::Header*>(raw_data);
        TEDS::Type teds_type = header->type;
        const void* teds_data = static_cast<const char*>(raw_data) + sizeof(TEDS::Header);

        print_bits(teds_type, "Parsed Full Type:   ");

        // Sensor receive flow
        if (TEDS::is_request(teds_type)) {
            auto* request = static_cast<const TEDS::RequestPayload*>(teds_data);
            TEDS::Period period = request->interval_ms + 1000;

            // std::cout << "\n[TEDS Handler] Received INTEREST (Period: " << period << "ms)."
            //           << std::endl;

            // print_bits(request->interval_ms, "Parsed Interval:    ");
            // std::cout << "Decimal Value: " << request->interval_ms << std::endl;
            // std::cout << "--- END DEBUG ---" << std::endl;

            // TODO: generic method do verify the bridges registered and call this (way later)
            _component_bridge.notify_interest_request(period, teds_type);

        // Actuator receive flow
        } else if (TEDS::is_response(teds_type)) {

            // auto* response = static_cast<const TEDS::ResponsePayload*>(teds_data);
            
            // // TOM DEBUG: unused variable
            // float value = response->value;
            
            // just apply in the actuator
            _component_bridge.apply_value_from_payload(msg.get_payload());
            
        } else {
            std::cout << "[TEDS Handler] Received unknown TEDS message type." << std::endl;
        }
    }

    /**
     * @warning NOT IN USE YET. USE THE SEND_INTEREST METHOD IN COMMUNICATOR INSTEAD.
     * @brief Sends a TEDS INTEREST message
     */
    // void send_interest(Communicator<LocalProtocol>& comm, Address dst, TEDS::Type base_type, TEDS::Period interval_ms)
    // {
    //     std::cout << "[TEDS Handler] Sending INTEREST for Type " << base_type << " (Period: " << interval_ms << "ms)" << std::endl;

    //     // 1. Build TEDS request payload (Header + RequestPayload)
    //     std::vector<char> teds_payload = TEDS::create_request_payload(base_type, interval_ms);

    //     // 2. Use the new generic Message constructor
    //     // WE NEED TO ADJUST THE TYPE HERE!!!
    //     Message interest_msg(dst, <type>, teds_payload);
    //     interest_msg.set_source(comm.address());
        
    //     comm.send(&interest_msg);
    // }

    /**
     * @brief Reactively sends a TEDS RESPONSE message
     */
    void send_response(Communicator<LocalProtocol>& comm, Address dest, TEDS::Type type) override
    {
        // Ask the bridge to get sensor data and package it into a payload
        // TO ADJUST THE VALUE. THE ARGUMENT type CAN TELL US WHAT PROGRAMMING TYPE THIS SHOULD BE
        float sensor_data = _component_bridge.get_value();

        std::cout << "[TEDS Handler] Sending RESPONSE to " << dest.paddr() << std::endl;

        if (sensor_data) {

            // the response payload is already being well formed in the Message constructor
            Message response_msg(dest, type, sensor_data);
            response_msg.set_source(comm.address());
            
            comm.send(&response_msg);

        } else {
            std::cout << "[TEDS Handler] I am not a Sensor, ignoring INTEREST." << std::endl;
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
    
};

#endif  // TEDS_HANDLER.HPP