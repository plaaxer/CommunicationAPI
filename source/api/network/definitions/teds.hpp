
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
    using Period = unsigned int;

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

    struct Header {
        Type type;
    } __attribute__((packed));


    struct ResponsePayload {
        float value;
    } __attribute__((packed));

    struct RequestPayload {
        Period interval_ms;
    } __attribute__((packed));

    
    inline std::vector<char> create_request_payload(Type base_type, Period interval_ms) {
        // 1. Create the header and payload data on the stack
        Header header{make_request_type(base_type)};
        RequestPayload payload{interval_ms};

        // 2. Create a vector to hold the combined data
        std::vector<char> buffer(sizeof(Header) + sizeof(RequestPayload));

        // 3. Copy the header and payload into the buffer
        std::memcpy(buffer.data(), &header, sizeof(Header));
        std::memcpy(buffer.data() + sizeof(Header), &payload, sizeof(RequestPayload));

        return buffer;
    }

    /**
     * @brief Builds a complete payload (Header + Data) for a TEDS response.
     */
    inline std::vector<char> create_response_payload(Type base_type, float value) {
        Header header{make_response_type(base_type)};
        ResponsePayload payload{value};

        std::vector<char> buffer(sizeof(Header) + sizeof(ResponsePayload));

        std::memcpy(buffer.data(), &header, sizeof(Header));
        std::memcpy(buffer.data() + sizeof(Header), &payload, sizeof(ResponsePayload));

        return buffer;
    }
}

#endif // TEDS_HPP