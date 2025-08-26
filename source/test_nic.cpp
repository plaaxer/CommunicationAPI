#include <iostream>
#include <unistd.h> // For sleep()
#include <cstring>
#include <cstdio>

#include "api/raw_socket_engine.hpp"
#include "api/network/ethernet.hpp"
#include "api/network/nic.hpp"
#include "api/network/protocol.hpp"
#include "api/network/traits.hpp"
#include "api/observer/conditional_data_observer.hpp"
#include "api/network/statistics.hpp"


// --- A simple Observer for testing the NIC ---
// This class will be notified by the NIC when a packet with our
// custom protocol number arrives.
// "typename" is required here to tell the compiler that Observer is a type.
class NIC_Observer : public Conditional_Data_Observer<Ethernet::Frame, uint16_t>
{
private:
    // This is the callback method that the NIC's receiver thread will call.
    // THE FIX: The signature of this method now exactly matches the one in the
    // base class, Conditional_Data_Observer<Ethernet::Frame, uint16_t>.
    // The first parameter must be a pointer to the base observed type.
    void update(Conditionally_Data_Observed<Ethernet::Frame, uint16_t>* obs,
                uint16_t prot,
                Ethernet::Frame* frame) override
    {
        // After we gonna put this message at the End-Point
        std::cout << "\n========== PACKET RECEIVED! ============" << std::endl;
        std::cout << "  Source MAC: " << Ethernet::MAC(frame->header.shost) << std::endl;
        std::cout << "  Protocol: 0x" << std::hex << prot << std::dec << std::endl;
        std::cout << "  Payload: \"";
        // Print the first few bytes of data as a string
        for(int i = 0; i < 30 && frame->data[i] != '\0'; ++i) {
            std::cout << frame->data[i];
        }
        std::cout << "\"" << std::endl;
        std::cout << "========================================\n" << std::endl;

        // Note: If you need to access methods specific to the NIC class from here,
        // you would need to safely cast the 'obs' pointer:
        // NIC<RawSocketEngine>* nic = dynamic_cast<NIC<RawSocketEngine>*>(obs);
        // if (nic) { /* ... use nic pointer safely ... */ }
    }
};

int main()
{
    try {
        // // to identify the Vehicle by setted environment variable (to implement yet)
        // char* vehicle_cstr = getenv("VEHICLE_ID");
        // std::string vehicle_id_str;
        // if (vehicle_cstr == nullptr) {
        //     std::cout << "Environment variable for the vehicle id not found." << std::endl;
        //     vehicle_id_str = "UNKNOWN";
        // } else {
        //     vehicle_id_str = std::string(vehicle_cstr);
        // }

        // std::cout << "This is vehicle: " << vehicle_id_str << std::endl;

        sleep(3);  // network interfaces need some time to be ready

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


        // 4. Send a broadcast message every 5 seconds.
        int message_count = 0;
        while(true) {
            std::string message = "Hello World!";

            Ethernet::MAC broadcast_addr;
            memset(broadcast_addr.addr, 0xFF, sizeof(broadcast_addr.addr));

            std::cout << "Sending broadcast message: \"" << message << "\"" << std::endl;

            nic.send(broadcast_addr,
                     Traits<Protocol<NIC<RawSocketEngine>>>::ETHERNET_PROTOCOL_NUMBER,
                     message.c_str(),
                     message.length() + 1); // +1 for null terminator

            sleep(5);  // Slowly for better view
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        std::cerr << "This program must be run with root privileges (sudo)." << std::endl;
        return 1;
    }

    return 0;
}