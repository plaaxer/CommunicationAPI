#ifndef I_SMART_DATA_HANDLER_BRIDGE_HPP
#define I_SMART_DATA_HANDLER_BRIDGE_HPP

#pragma once
#include <vector>

class ISmartDataHandlerBridge {
public:
    virtual ~ISmartDataHandlerBridge() = default;
    
    // TO ADJUSTE THE TYPE CORRECTLY LATER
    virtual uint32_t get_value() = 0;
    
    // Called by handler on a TEDS::RESPONSE message
    virtual void set_value_from_payload(const std::vector<char>& payload) = 0;
};

#endif  // I_SMART_DATA_HANDLER_BRIDGE.HPP