#ifndef PROTOCOL_UTILS_HPP
#define PROTOCOL_UTILS_HPP

#include <iostream>
#include <cstdio>
#include "api/network/definitions/buffer.hpp"
#include "api/network/definitions/address.hpp"
#include "api/network/definitions/traits.hpp"
#include "api/network/nic.hpp"

// Forward declare Protocol so we can access nested types
template <typename LocalNIC, typename ExternalNIC> class Protocol;

/**
 * @brief Utility functions for debugging Protocol frames and packets.
 */
namespace ProtocolUtils {

    // The LocalNIC template parameter should match your Protocol instantiation
    template <typename LocalNIC, typename ExternalNIC = void>
    void log_receiving(typename Protocol<LocalNIC, ExternalNIC>::Buffer* buf);

    template <typename LocalNIC, typename ExternalNIC = void>
    void debug_frame(typename Protocol<LocalNIC, ExternalNIC>::Buffer* buf);

} // namespace ProtocolUtils

#endif // PROTOCOL_UTILS_HPP
