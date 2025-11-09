#ifndef I_GROUP_HANDLER_HPP
#define I_GROUP_HANDLER_HPP

#include "api/network/definitions/message.hpp"
#include "api/network/definitions/address.hpp"
#include <cstdint>



template<typename Engine> class NIC;
class ShmEngine;
class RawSocketEngine;
template <typename LocalNIC, typename ExternalNIC = void> class Protocol;

using LocalNIC = NIC<ShmEngine>;
using ExternalNIC = NIC<RawSocketEngine>;

// -----------------------------


class IGroupHandler {
public:
    using SessionKey = uint64_t;

    virtual ~IGroupHandler() = default;

    // here we are going to have messages such as JOIN, or receiving a session key (in the case of grup members)
    virtual void handle_group_message(Protocol<LocalNIC, ExternalNIC>, const Message& msg) = 0;

    // the session key should be at the shared memory.
    virtual SessionKey get_session_key() const = 0;
};

#endif // I_GROUP_HANDLER_HPP