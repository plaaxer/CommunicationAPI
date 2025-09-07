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

using RseNIC = NIC<RawSocketEngine>;
using SmhNIC = NIC<SmhEngine>;
using MyProtocol = Protocol<<SmhNIC>,<RseNIC>>;

// The port our RCU component will use
constexpr int RCU_PORT = 7090;

int main() {
    try {
        // 1. Initialize both NICs.
        RseNIC rseNIC;
        SmhNIC smhNIC;

        // 2. Instantiates static instance of the Protocol Handler
        MyProtocol::instance().init(&rseNIC, &smhNIC);

        // 3. Creating the Communicator for RCU component
        Address my_address(nic.address(), RCU_PORT);
        Communicator<MyProtocol> general_communicator(&MyProtocol::instance(), my_address);
        std::cout << "[Component Process] RCU Communicator created for port " << RCU_PORT << "." << std::endl;

        while(true) {
            std::string content;
            // 4. Wait to recieve a message
            Message received_msg = Message(30);
            if (general_communicator->receive(&received_msg)) {
                content = static_cast<char*>(received_msg.data());
            }
            // 5. Broadcast it (TODO: Add parsing, resend message with specific port in mind)
            Message message_to_send(content);
            general_communicator.send(&message_to_send);
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        std::cerr << "This program must be run with root privileges (sudo)." << std::endl;
        return 1;
    }

    return 0;
}