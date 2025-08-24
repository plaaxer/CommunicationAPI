#include <iostream>
#include <unistd.h> // For sleep()
#include <cstring>

#include "api/rawsocketengine.h"
#include "api/network/ethernet.hpp"
#include "api/network/nic.hpp"
#include "api/network/traits.hpp"
#include "api/observer/conditional_data_observer.hpp"
#include "api/network/statistics.hpp"
#include "api/network/buffer.hpp"


// --- A simple Observer for testing the NIC ---
class NIC_Observer : public NIC<RawSocketEngine>::Observer
{
public:
    // NIC's receiver thread will call this method when a packet with the
    // registered protocol number is received.
    void update(typename NIC<RawSocketEngine>::Observed* obs,
                typename NIC<RawSocketEngine>::Protocol_Number prot,
                typename NIC<RawSocketEngine>::Frame& frame) override
    {
        std::cout << "\n=== PACKET RECEIVED! (in NIC_Observer) ===" << std::endl;
        std::cout << "  Protocol: 0x" << std::hex << prot << std::dec << std::endl;
        std::cout << "  Source MAC: " << Ethernet::Address(frame.header.shost) << std::endl;
        std::cout << "  Payload: \"";
        // Print the first few bytes of data as a string
        for(int i = 0; i < 20 && frame.data[i] != '\0'; ++i) {
            std::cout << frame.data[i];
        }
        std::cout << "\"" << std::endl;
        std::cout << "========================================\n" << std::endl;
    }
};


int main()
{
    try {
        // 1. Instantiate the NIC. This will start its receiver thread.
        NIC<RawSocketEngine> nic;

        // 2. Instantiate our test observer.
        NIC_Observer my_observer;

        // 3. Attach the observer to the NIC.
        // We are telling the NIC: "Please notify 'my_observer' whenever you
        // receive a packet with our custom protocol number."
        // We get the protocol number from the Traits file.
        nic.attach(&my_observer, Traits<Protocol<NIC<RawSocketEngine>>>::ETHERNET_PROTOCOL_NUMBER);
        std::cout << "Test Observer attached for protocol 0x"
                  << std::hex << Traits<Protocol<NIC<RawSocketEngine>>>::ETHERNET_PROTOCOL_NUMBER << std::dec
                  << std::endl;


        // 4. Send a broadcast message every 3 seconds.
        int message_count = 0;
        while(true) {
            std::string message = "Hello from " + std::to_string(message_count++);
            
            // The broadcast MAC address is FF:FF:FF:FF:FF:FF
            Ethernet::Address broadcast_addr;
            memset(broadcast_addr.addr, 0xFF, sizeof(broadcast_addr.addr));

            std::cout << "Sending broadcast message: \"" << message << "\"" << std::endl;

            nic.send(broadcast_addr,
                     Traits<Protocol<NIC<RawSocketEngine>>>::ETHERNET_PROTOCOL_NUMBER,
                     message.c_str(),
                     message.length() + 1); // +1 for null terminator

            sleep(3);
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        std::cerr << "This program must be run with root privileges (sudo)." << std::endl;
        return 1;
    }

    return 0;
}
