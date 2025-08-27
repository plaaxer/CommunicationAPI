#ifndef ENGINE_ECU_HPP
#define ENGINE_ECU_HPP

#include <iostream>

#include "api/observer/conditional_data_observer.hpp"
#include "components/engine.hpp"
#include "api/network/communicator.hpp"
#include "api/message.hpp"
#include "api/raw_socket_engine.hpp"
#include "api/network/nic.hpp"
#include "api/network/protocol.hpp"
#include "api/network/address.hpp"

// Forward declare to avoid circular dependencies
template <typename NIC> class Protocol;
using MyNIC = NIC<RawSocketEngine>; // Assuming RawSocketEngine
using MyProtocol = Protocol<MyNIC>;

// The ECU now only observes EngineData
class EngineECU : public Conditional_Data_Observer<EngineData, void>
{
public:
    EngineECU() : _comm(nullptr) {
        // The Communicator is the ECU's dedicated network endpoint
        _comm = new Communicator<MyProtocol>(&MyProtocol::instance(), Address(Address::Port(10001)));
    }

    ~EngineECU() {
        delete _comm;
    }

    // This is called by the EngineComponent when it has new data
    void update(Conditionally_Data_Observed<EngineData, void>* obs, EngineData* data) override {
        std::cout << "[ECU] Received new Engine data: RPM=" << data->rpm
                  << ", Temp=" << (int)data->temperature_celsius << "C" << std::endl;
        _engine_data = *data;

        // After receiving data from the component, broadcast it over the network
        broadcast_status();
    }

private:
    void broadcast_status() {
        // 1. Consolidate data into a message
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Engine Status: RPM=%d, Temp=%dC",
                 _engine_data.rpm, _engine_data.temperature_celsius);
        
        Message ecu_message(buffer);
        std::cout << "[ECU] Broadcasting network message: \"" << buffer << "\"" << std::endl;

        // 2. Send the message via the Communicator
        // This triggers the full send call stack: Communicator -> Protocol -> NIC -> Engine
        _comm->send(&ecu_message);
    }

private:
    Communicator<MyProtocol>* _comm;
    EngineData _engine_data;
};

#endif // ENGINE_CONTROL_UNIT_HPP