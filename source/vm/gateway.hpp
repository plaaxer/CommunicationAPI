#ifndef GATEWAY_HPP
#define GATEWAY_HPP

#include <iostream>
#include <thread>
#include <chrono>

#include "api/network/engines/raw_socket_engine.hpp"
#include "api/network/engines/smh_engine.hpp"
#include "api/network/nic.hpp"
#include "api/network/protocol.hpp"
#include "api/network/ptp/ptp_roles.hpp"

// The NIC for internal communication with other processes on the same machine
using InternalNIC = NIC<ShmEngine>;

// The NIC for external communication over the network
using ExternalNIC = NIC<RawSocketEngine>;

// The special protocol that bridges the two NICs
using GatewayProtocol = Protocol<InternalNIC, ExternalNIC>;

class Gateway {
public:
    Gateway(PtpRole role) : _ptp_role(role) {  
        
        if (role == PtpRole::SLAVE) {
            std::cout << "--- Initializing Gateway RCU ---" << std::endl;
        } else if (role == PtpRole::MASTER) { 
            std::cout << "--- Initializing Roadside Unit's Gateway ---" << std::endl;
        }
            
        GatewayProtocol::init_gateway(&_internal_nic, &_external_nic, role);
                
        if (role == PtpRole::SLAVE) {
            std::cout << "--- Gateway RCU Initialized Successfully ---" << std::endl;
        } else if (role == PtpRole::MASTER){ 
            std::cout << "--- Roadside Unit's Gateway Initialized Successfully ---" << std::endl;
        }
    }

    void run() {

        if (_ptp_role == PtpRole::SLAVE) {
            std::cout << "Gateway is running. Forwarding is active." << std::endl;
            while (true) {
                // Keep the process alive with minimal CPU usage.
                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
        } else {
            while (true) {
                // std::cout << "[RSU] Current global time: " << Clock::getCurrentTimeString() << std::endl;

                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        }

    }

private:
    InternalNIC _internal_nic;
    ExternalNIC _external_nic;

    PtpRole _ptp_role;
};

#endif // GATEWAY_HPP