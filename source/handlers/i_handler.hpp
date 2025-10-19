#ifndef I_HANDLER_HPP
#define I_HANDLER_HPP

#pragma once

#include "api/network/communicator.hpp"
#include "api/network/definitions/message.hpp"
#include "api/network/protocol.hpp"
#include "api/network/engines/smh_engine.hpp" // For NIC/ShmEngine
#include "api/network/nic.hpp" // For NIC

// --- Define the concrete types your handlers will use ---
// This ensures the interfaces use the same types as your Component.
using LocalNIC = NIC<ShmEngine>;
using LocalProtocol = Protocol<LocalNIC>;

/**
 * @brief Interface for any class that can handle TEDS messages.
 */
class ITEDSHandler {
public:
    virtual ~ITEDSHandler() = default;

    /**
     * @brief The main entry point for processing a TEDS message.
     * @param comm The communicator, to allow the handler to send replies.
     * @param msg The received message containing the payload and source address.
     */
    virtual void handleTEDSMessage(Communicator<LocalProtocol>& comm, const Message& msg) = 0;
};

/**
 * @brief Interface (contract) for any class that can handle CONTROL messages.
 */
class IControlHandler {
public:
    virtual ~IControlHandler() = default;

    /**
     * @brief The main entry point for processing a CONTROL message.
     * @param comm The communicator, to allow the handler to send replies.
     * @param msg The received message containing the payload and source address.
     */
    virtual void handleControlMessage(Communicator<LocalProtocol>& comm, const Message& msg) = 0;
};

#endif  // I_HANDLER.HPP