#include <iostream>
#include <thread>
#include <chrono>

#include "api/raw_socket_engine.hpp"
#include "api/network/nic.hpp"
#include "api/network/protocol.hpp"
#include "components/engine.hpp"
#include "controllers/engine_ecu.hpp"

// Define concrete types for the network stack
using MyNIC = NIC<RawSocketEngine>;
using MyProtocol = Protocol<MyNIC>;

// Specialization for the unconditional attach method of the observer
template<>
void Conditionally_Data_Observed<EngineData, void>::attach(Observer* o) {
    _observers.insert(_observers.begin(), o);
}

int main() {
    try {
        std::cout << "--- Starting Autonomous Vehicle Simulation ---" << std::endl;
        sleep(2); // Give the network interface a moment

        // 1. Initialize the Network Stack (using Singletons)
        MyNIC& nic = MyNIC::getInstance();
        MyProtocol::getInstance().init(&nic);

        // 2. Create the Vehicle Components
        Engine engine;

        // 3. Create the Engine Control Unit (ECU)
        EngineECU ecu;

        // 4. Link the ECU as an observer to the Engine component
        engine.attach(&ecu);
        std::cout << "\n[System] ECU is now observing the Engine." << std::endl;

        // 5. Run the main simulation loop
        std::cout << "[System] Starting main simulation loop...\n" << std::endl;
        while (true) {
            // Every 2 seconds, trigger the engine to generate new data.
            // This will automatically notify the ECU, which will then broadcast a message.
            engine.generate_new_data();
            
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        std::cerr << "This program must be run with root privileges (sudo)." << std::endl;
        return 1;
    }

    return 0;
}