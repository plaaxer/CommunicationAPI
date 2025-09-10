#include <iostream>
#include <thread>
#include <chrono>
#include <string>

// Core network stack
#include "api/network/engines/raw_socket_engine.hpp"
#include "api/network/engines/smh_engine.hpp"
#include "api/network/nic.hpp"
#include "api/network/protocol.hpp"
#include "api/network/communicator.hpp"
#include "api/network/definitions/message.hpp"

using LocalNIC = NIC<RawSocketEngine>;
using ExternalNIC = NIC<ShmEngine>;
using MyProtocol = Protocol<LocalNIC, ExternalNIC>;


// THIS FILE IS SIMILAR TO THE START_CAR ONE, BUT USES A PROTOCOL WITH DUAL NIC INSTEAD.
// KEEP IN MIND THAT THE NICS ABOVE ARE INVERTED FROM AN ACTUAL APP!!!!!!!!!!!!

constexpr int ENGINE_PORT = 9001;

void receiver_loop(Communicator<MyProtocol>* comm) {
    std::cout << "[Receiver Thread] Started. Waiting for messages on port " << ENGINE_PORT << "..." << std::endl;
    
    while (true) {
        Message received_msg = Message(30);

        if (comm->receive(&received_msg)) {
            std::cout << "\n[Receiver Thread] ===> MESSAGE RECEIVED: "
                      << static_cast<char*>(received_msg.data()) << std::endl;
        }
    }
}

/**
 * @brief The main entry point for the vehicle simulation.
 */
int main() {
    try {
        std::cout << "--- Starting Vehicle Simulation ---" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        ExternalNIC external_nic;
        LocalNIC local_nic;

        MyProtocol::init_gateway(&local_nic, &external_nic);

        Address my_address(local_nic.address(), ENGINE_PORT);
        Communicator<MyProtocol> engine_communicator(&MyProtocol::instance(), my_address);
        std::cout << "[Main Thread] Engine Communicator created for port " << ENGINE_PORT << "." << std::endl;

        std::thread receiver_thread(receiver_loop, &engine_communicator);
        receiver_thread.detach();

        int message_count = 0;
        while (true) {
            std::string content = "Engine Broadcast #" + std::to_string(message_count++);
            Message message_to_send(content);

            std::cout << "[Main Thread] Sending: '" << content << "'" << std::endl;
            
            engine_communicator.send(&message_to_send);

            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        // std::cerr << "This program must be run with root privileges (sudo)." << std::endl;
        return 1;
    }

    return 0;
}
