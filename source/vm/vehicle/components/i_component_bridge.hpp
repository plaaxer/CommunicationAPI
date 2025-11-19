#ifndef I_COMPONENT_BRIDGE_HPP
#define I_COMPONENT_BRIDGE_HPP

#include "api/network/definitions/teds.hpp"
#include "api/network/definitions/address.hpp"

#pragma once
template<typename LocalSmartData>
class IComponentBridge
{
    using Period = typename TEDS::Period;
    using Value  = typename LocalSmartData::Value;

public:
    virtual ~IComponentBridge() = default;
    
    virtual Value get_value() = 0;
    
    virtual void apply_value_from_payload(const std::vector<char>& payload) = 0;

    virtual void notify_interest_request(Period requested_interval, TEDS::Type type, bool reset = false) = 0;

    virtual bool is_entity_nearby(const Address& addr) = 0;

};

#endif  // COMPONENT_BRIDGE_HPP