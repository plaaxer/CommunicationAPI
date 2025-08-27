#include <iostream>
#include <thread>
#include <chrono>
#include <string>

// Core network stack
#include "api/raw_socket_engine.hpp"
#include "api/network/nic.hpp"
#include "api/network/protocol.hpp"
#include "api/network/communicator.hpp"
#include "api/message.hpp"

using MyNIC = NIC<RawSocketEngine>;
using MyProtocol = Protocol<MyNIC>;

// The port our Engine component will use
constexpr int ENGINE_PORT = 9001;

/**
 * @brief This function runs in a dedicated thread, acting as the message receiver.
 */
void receiver_loop(Communicator<MyProtocol>* comm) {
    std::cout << "[Receiver Thread] Started. Waiting for messages on port " << ENGINE_PORT << "..." << std::endl;
    
    while (true) {
        Message received_msg = Message(30);

        // Adjust this call if your Communicator::receive signature differs.
        // Many implementations provide receive(Message*, Address*) or receive(Message*).
        if (comm->receive(&received_msg)) {
            std::cout << "\n[Receiver Thread] ===> MESSAGE RECEIVED: '"
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
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Give the network interface a moment

        // 1. Initialize the NIC with your _receiver_thread listening recv
        MyNIC nic;

        // 2. Instantiates static instance of the Protocol Handler
        MyProtocol::instance().init(&nic);

        // 3. Creating the Communicator for the Vehicle Engine component
        Address my_address(nic.address(), ENGINE_PORT);
        Communicator<MyProtocol> engine_communicator(&MyProtocol::instance(), my_address);
        std::cout << "[Main Thread] Engine Communicator created for port " << ENGINE_PORT << "." << std::endl;

        // 4. Launch the dedicated Vehicle Engine receiver thread
        std::thread receiver_thread(receiver_loop, &engine_communicator);
        receiver_thread.detach(); // let it run independently while main does sending

        // 5. Run the main sender loop in the main thread
        std::cout << "[Main Thread] Starting sender loop..." << std::endl;
        int message_count = 0;
        while (true) {
            std::string content = "Engine Broadcast #" + std::to_string(message_count++);
            Message message_to_send(content);

            std::cout << "[Main Thread] Sending: '" << content << "'" << std::endl;
            
            // This triggers the full, synchronous send call stack:
            // Communicator -> Protocol -> NIC -> Engine
            engine_communicator.send(&message_to_send);

            std::this_thread::sleep_for(std::chrono::seconds(3));
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        std::cerr << "This program must be run with root privileges (sudo)." << std::endl;
        return 1;
    }

    return 0;
}