#ifndef GATEWAY_HPP
#define GATEWAY_HPP

#include <iostream>
#include <thread>
#include <chrono>

#include "api/network/engines/raw_socket_engine.hpp"
#include "api/network/engines/smh_engine.hpp"
#include "api/network/nic.hpp"
#include "api/network/protocol.hpp"
#include "utils/profiler.hpp"

// The NIC for internal communication with other processes on the same machine
using InternalNIC = NIC<ShmEngine>;

// The NIC for external communication over the network
using ExternalNIC = NIC<RawSocketEngine>;

// The special protocol that bridges the two NICs
using GatewayProtocol = Protocol<InternalNIC, ExternalNIC>;


class Gateway {
public:
    Gateway(Profiler *p) {
        std::cout << "--- Initializing Gateway RCU ---" << std::endl;
        
        GatewayProtocol::init_gateway(&_internal_nic, &_external_nic);

        std::cout << "--- Gateway RCU Initialized Successfully ---" << std::endl;
    }

    // The gateway's main job is to simply exist and keep the protocol stack alive.
    void run() {
        std::cout << "Gateway is running. Forwarding is active." << std::endl;
        while (true) {
            // Keep the process alive with minimal CPU usage.
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    }

private:
    InternalNIC _internal_nic;
    ExternalNIC _external_nic;
};

#endif // GATEWAY_HPP