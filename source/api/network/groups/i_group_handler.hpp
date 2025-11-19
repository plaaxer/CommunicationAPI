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

    virtual void heartbeat_update(const Address& client) {};
};

namespace std {
    template<>
    struct hash<Address> {
        std::size_t operator()(const Address& addr) const {
            // Combine hash of internal members
            // This example assumes Address has a public 'mac' array or getter
            // You might need to adjust 'addr.mac' to whatever your internal variable is.
            
            // FNV-1a hash algorithm (simple and fast for byte arrays)
            size_t hash = 2166136261u;
            const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&addr);
            for (size_t i = 0; i < sizeof(Address); ++i) {
                hash ^= ptr[i];
                hash *= 16777619u;
            }
            return hash;
        }
    };
}

#endif // I_GROUP_HANDLER_HPP