#ifndef TEDS_HANDLER_HPP
#define TEDS_HANDLER_HPP

#pragma once

#include "handlers/i_handler.hpp"

#include "vehicle/smartdata/i_smart_data_handler_bridge.hpp"
#include "api/network/protocol.hpp"
#include "api/network/definitions/teds.hpp"

#include <iostream>

using LocalProtocol = Protocol<NIC<ShmEngine>>;

class TEDSHandler : public ITEDSHandler {
public:
    TEDSHandler(ISmartDataHandlerBridge& bridge)
        : _smart_data_bridge(bridge) 
    {}

    virtual void handleTEDSMessage(Communicator<LocalProtocol>& comm, const Message& msg) override 
    {
        const char* payload_data = static_cast<const char*>(msg.data());
        auto* header = reinterpret_cast<const TEDS::Header*>(payload_data);

        TEDS::Type teds_type = header->type;  // info about request/response, data type, etc.
        const void* teds_data = static_cast<const char*>(payload_data) + sizeof(TEDS::Header);

        // Sensor receive flow
        if (TEDS::is_request(teds_type)) {
            auto* request = static_cast<const TEDS::RequestPayload*>(teds_data);
            TEDS::Period period = request->interval_ms;

            std::cout << "\n[TEDS Handler] Received INTEREST (Period: " << period << "ms)."
                      << std::endl;

            // For now, we just send one response back in BROADCAST
            send_response(comm, Address::broadcast(LocalProtocol::TYPE_BASED_ROUTING_PORT), 
                            teds_type);

        // Actuator receive flow
        } else if (TEDS::is_response(teds_type)) {

            auto* response = static_cast<const TEDS::ResponsePayload*>(teds_data);
            
            // TOM DEBUG: unused variable
            float value = response->value;
            
            // just apply in the actuator
            _smart_data_bridge.set_value_from_payload(msg.get_payload());
            
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

private:
    /**
     * @brief Reactively sends a TEDS RESPONSE message
     */
    void send_response(Communicator<LocalProtocol>& comm, Address dest, TEDS::Type type)
    {
        // Ask the bridge to get sensor data and package it into a payload
        // TO ADJUST THE VALUE. THE ARGUMENT type CAN TELL US WHAT PROGRAMMING TYPE THIS SHOULD BE
        float sensor_data = _smart_data_bridge.get_value();

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

private:
    ISmartDataHandlerBridge& _smart_data_bridge;
};

#endif  // TEDS_HANDLER.HPP