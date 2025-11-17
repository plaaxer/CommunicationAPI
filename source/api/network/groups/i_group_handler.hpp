#ifndef I_GROUP_HANDLER_HPP
#define I_GROUP_HANDLER_HPP

#include "api/network/definitions/message.hpp"
#include "api/network/definitions/address.hpp"
#include "api/network/crypto/i_crypto_provider.hpp"
#include <cstdint>

template<typename Engine> class NIC;
class ShmEngine;
class RawSocketEngine;
template <typename LocalNIC, typename ExternalNIC> class Protocol;

using LocalNIC = NIC<ShmEngine>;
using ExternalNIC = NIC<RawSocketEngine>;

template<typename LocalNIC, typename ExternalNIC>
class IGroupHandler {
public:

    virtual ~IGroupHandler() = default;

    /**
     * @brief Handles group messages such as join and key distribution.
     */
    virtual void handle_group_message(const void* payload, size_t size, const Address& from) = 0;

    /**
     * @brief Occurs when the car changes quadrants. Doesn't happen with group leaders.
     */
    virtual void notify_location_change() {};
};

#endif // I_GROUP_HANDLER_HPP