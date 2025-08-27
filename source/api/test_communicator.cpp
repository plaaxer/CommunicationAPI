
// =============================================================================
// Main Application Includes
// =============================================================================
#include "api/network/nic.hpp"
#include "api/network/protocol.hpp"
#include "api/network/communicator.hpp"
#include "api/raw_socket_engine.hpp"
#include "api/message.hpp"

// =============================================================================
// Test Logic
// =============================================================================

// Define type aliases for our network stack for simplicity
using MyNIC = NIC<RawSocketEngine>;
using MyProtocol = Protocol<MyNIC>;
using MyCommunicator = Communicator<MyProtocol>;

// We need to define the static observed member for the Protocol
template<> MyProtocol::Observed MyProtocol::_observed;

// This function will run in a separate thread to handle receiving
void receiver_task(MyCommunicator* comm, const std::string& expected_message) {
    std::cout << "[Receiver Thread] Waiting for a message..." << std::endl;

    // Create a message buffer to store the incoming data
    Message received_msg(1024);

    if (comm->receive(&received_msg)) {
        std::string received_str(static_cast<char*>(received_msg.data()), received_msg.size());
        std::cout << "[Receiver Thread] SUCCESS: Message received!" << std::endl;
        std::cout << "  > Received: \"" << received_str << "\"" << std::endl;
        
        if (received_str == expected_message) {
            std::cout << "  > VERIFICATION: OK! ✅" << std::endl;
        } else {
            std::cout << "  > VERIFICATION: FAILED! ❌" << std::endl;
        }
    } else {
        std::cout << "[Receiver Thread] FAILURE: Receive call failed." << std::endl;
    }
}

int main() {
    std::cout << "--- Network Communicator Test ---" << std::endl;

    // 1. Initialize the network stack
    MyNIC nic; // The NIC constructor starts its receiver thread
    MyProtocol::init(&nic); // Initialize the protocol singleton with our NIC

    // 2. Define addresses for two endpoints
    // Both will use the NIC's MAC address but different ports.
    Address addr_sender(nic.address(), 1000);
    Address addr_receiver(nic.address(), 2000);
    
    // 3. Create Communicator instances for sending and receiving
    MyCommunicator comm_sender(&MyProtocol::instance(), addr_sender);
    MyCommunicator comm_receiver(&MyProtocol::instance(), addr_receiver);

    // 4. Start the receiver task in a new thread
    std::string message_content = "Hello Network!";
    std::thread receiver_thread(receiver_task, &comm_receiver, message_content);

    // Give the receiver thread a moment to start up and block on receive()
    std::cout << "[Main Thread] Waiting 1 second before sending..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 5. Send a message from the main thread
    std::cout << "[Main Thread] Sending message: \"" << message_content << "\"" << std::endl;
    Message msg_to_send(message_content);

    // We send TO the receiver's address. The `from` address is handled by the communicator.
    // We will send to the broadcast address to ensure the NIC picks it up in this simple test.
    if (MyProtocol::send(addr_sender, Address::broadcast(), msg_to_send.data(), msg_to_send.size()) > 0) {
        std::cout << "[Main Thread] Send command issued successfully." << std::endl;
    } else {
        std::cout << "[Main Thread] Send command failed." << std::endl;
    }

    // 6. Wait for the receiver thread to finish its job
    receiver_thread.join();

    std::cout << "--- Test Finished ---" << std::endl;

    return 0;
}