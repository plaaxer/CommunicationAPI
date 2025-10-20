#ifndef I_COMPONENT_BRIDGE_HPP
#define I_COMPONENT_BRIDGE_HPP

#include "api/network/definitions/teds.hpp"

#pragma once

class IComponentBridge
{
    typedef typename TEDS::Period Period;

public:
    virtual ~IComponentBridge() = default;
    
    virtual uint32_t get_value() = 0;
    
    virtual void apply_value_from_payload(const std::vector<char>& payload) = 0;

    virtual void notify_interest_request(Period requested_interval, TEDS::Type type);
};

#endif  // COMPONENT_BRIDGE_HPP