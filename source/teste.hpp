#include "api/network/communicator.hpp"
#include "api/network/protocol.hpp"
#include "api/network/engines/smh_engine.hpp" // Or your chosen engine
#include "api/network/definitions/message.hpp"
#include "api/network/definitions/address.hpp"
#include "api/network/definitions/teds.hpp"

#include <iostream>
#include <thread>
#include <chrono>

// Define the TEDS types we'll use in this application
const TEDS::Type TEDS_TEMPERATURE = 0x1A001002;

// --- Component 1: The Producer ---
void temperature_sensor_main() {
    // 1. Setup the communication stack
    Protocol<NIC<ShmEngine>> protocol = Protocol<NIC<ShmEngine>>::instance();
    Address sensor_address(1001); // Its own unique port for control messages
    Communicator<decltype(protocol)> communicator(&protocol, sensor_address);

    std::cout << "[Sensor] 🌡️ Temperature Sensor online. Listening for interest requests." << std::endl;

    // 2. Subscribe to INTEREST REQUESTS for the data it produces.
    communicator.subscribe_to_requests(TEDS_TEMPERATURE);

    // Main loop
    bool has_interest = false;
    TEDS::Period publication_period_ms = 0;
    auto last_sent_time = std::chrono::steady_clock::now();
    float current_temp = 22.5f;

    while (true) {
        Message received_message;
        // This receive will block until a control message or an interest request arrives.
        if (communicator.receive(&received_message)) {
            
            if (received_message.get_type() == Segment::MsgType::TEDS) {
                // It's a TEDS message, let's parse the payload
                auto* interest_payload = reinterpret_cast<const TEDS::InterestPayload*>(received_message.data());
                
                // Check if it's a request for our data type
                if (TEDS::is_request(interest_payload->type) && TEDS::get_base_type(interest_payload->type) == TEDS_TEMPERATURE) {
                    std::cout << "[Sensor] 🌡️ Received interest in temperature data! Starting periodic publication." << std::endl;
                    has_interest = true;
                    publication_period_ms = interest_payload->interval_ms;
                }
            }
        }

        // 3. If someone is interested, periodically send data responses
        if (has_interest) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_sent_time).count();

            if (elapsed_ms >= publication_period_ms) {
                // Create a "smart" message with the data
                Message response_message(Address::broadcast(Protocol::TYPE_BASED_ROUTING_PORT), TEDS_TEMPERATURE, current_temp);
                communicator.send(&response_message);

                std::cout << "[Sensor] 🌡️ Sent Temperature: " << current_temp << " C" << std::endl;

                last_sent_time = now;
                current_temp += 0.1f; // Simulate temperature change
            }
        }
    }
}


// --- Component 2: The Consumer ---
void climate_controller_main() {
    // 1. Setup the communication stack
    Protocol<NIC<ShmEngine>> protocol = Protocol<NIC<ShmEngine>>::instance();
    Address controller_address(1002); // Its own unique port
    Communicator<decltype(protocol)> communicator(&protocol, controller_address);

    std::cout << "[Controller] ❄️ Climate Controller online." << std::endl;

    // 2. Subscribe to DATA RESPONSES for temperature, requesting updates every 1000ms.
    // This automatically sends the initial interest message.
    communicator.subscribe_to_responses(TEDS_TEMPERATURE, 1000);
    std::cout << "[Controller] ❄️ Sent interest request for temperature data." << std::endl;

    // Main loop
    while (true) {
        Message received_message;
        // This receive will block until a relevant message arrives (a temperature response)
        if (communicator.receive(&received_message)) {
            
            if (received_message.get_type() == Segment::MsgType::TEDS) {
                // It's a TEDS message, parse the payload to get the value
                auto* response_payload = reinterpret_cast<const TEDS::ResponsePayload*>(received_message.data());
                
                // Check if it's a response for our data type
                if (TEDS::is_response(response_payload->type) && TEDS::get_base_type(response_payload->type) == TEDS_TEMPERATURE) {
                    float temp_value = response_payload->value;
                    std::cout << "[Controller] ❄️ Received Temperature: " << temp_value << " C" << std::endl;

                    // 3. Application logic
                    if (temp_value > 25.0f) {
                        std::cout << "[Controller] ❄️ It's getting hot! Turning on the A/C." << std::endl;
                    }
                }
            }
        }
    }
}


// --- Main Application Entry Point ---
int main() {
    // Initialize the underlying communication engine and protocol (singleton)
    NIC<ShmEngine> nic;
    Protocol<NIC<ShmEngine>>::init_component(&nic);

    std::cout << "--- Starting Vehicle Component Simulation ---" << std::endl;

    // Run both components in separate threads to simulate concurrency
    std::thread sensor_thread(temperature_sensor_main);
    std::thread controller_thread(climate_controller_main);

    sensor_thread.join();
    controller_thread.join();

    return 0;
}