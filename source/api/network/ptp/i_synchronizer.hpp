
#ifndef I_SYNCHRONIZER_HPP
#define I_SYNCHRONIZER_HPP

#include "api/network/protocol.hpp"

class ISynchronizer {
public:
    virtual ~ISynchronizer() = default;
    
    virtual void handle_ptp_message(const void* payload, size_t size, 
                                    const Address& from, const Address& to) = 0;
    
    virtual void request_synchronization(const Address& my_address) {
        // (empty for masters)
    }
};

#endif // I_SYNCHRONIZER_HPP
