
#ifndef TEDS_HPP
#define TEDS_HPP

#include <cstdint>
#include <iostream>

/**
 * @details Our custom TEDS is based on a 32-bit unsigned integer.
 * The most significant bit (MSB) indicates whether the message is a request (0)
 * or a response (1). The remaining 31 bits represent the actual type of the message.
 */

namespace TEDS {
    using Type = uint32_t;

    // A mask to isolate the MSB (the "purpose" bit)
    constexpr Type PURPOSE_MASK = 0x80000000;
    // A mask to get just the base type
    constexpr Type TYPE_MASK    = 0x7FFFFFFF;

    inline bool is_response(Type t) {
        return (t & PURPOSE_MASK) != 0; // Is the MSB a 1?
    }

    inline bool is_request(Type t) {
        return (t & PURPOSE_MASK) == 0; // Is the MSB a 0?
    }

    inline Type get_base_type(Type t) {
        return t & TYPE_MASK; // Get the type without the purpose bit
    }

    inline Type make_response_type(Type base_type) {
        return base_type | PURPOSE_MASK; // Set the MSB to 1
    }

    inline Type make_request_type(Type base_type) {
        return base_type & TYPE_MASK; // Set the MSB to 0 (or just return the base type)
    }
    /**
     * @brief Builds a payload for a TEDS request.
     */
    inline std::vector<char> create_request_payload(Type type, unsigned int interval_ms) {
        struct RequestPayload {
            Type type;
            unsigned int interval;
        } __attribute__((packed));

        // Create the payload on the stack
        RequestPayload payload{make_request_type(type), interval_ms};

        // Copy the raw bytes of the payload into a char vector
        const char* start = reinterpret_cast<const char*>(&payload);
        const char* end = start + sizeof(RequestPayload);
        return std::vector<char>(start, end);
    }
}

#endif // TEDS_HPP