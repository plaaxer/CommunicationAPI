
#ifndef TEDS_HPP
#define TEDS_HPP

#include <cstdint>
#include <iostream>

/**
 * @details Our custom TEDS is based on a 32-bit unsigned integer.
 * The most significant bit (MSB) indicates whether the message is a request (0)
 * or a response (1). The remaining 31 bits represent the actual type of the message.
 */

    // A mask to isolate the MSB (the "purpose" bit)
     /* Custom TEDS Format:
    * ----------------------------------------------- *
    * | bit 31       | Request/Response | (1 bit)   |
    * ----------------------------------------------- *
    * | bit 30       | Digital/SI Unit  | (1 bit)   |
    * ----------------------------------------------- *
    * | bit 29       | Direct/Inverse   | (1 bit)   |
    * ----------------------------------------------- *
    * | bits 28 - 27 | Float/Int        | (2 bits)  |
    * ----------------------------------------------- *
    * | bits 26 - 0  | Base Type        | (27 bits) |
    * ----------------------------------------------- *
*/

namespace TEDS {
    using Type = uint32_t;

    // A mask to isolate the MSB (the "purpose" bit)
    constexpr Type PURPOSE_MASK = 0x80000000;
    // A mask to get digital/SI type
    constexpr Type DIGITAL_MASK = 0x40000000;
    // A mask to get if data is inverted
    constexpr Type INVERT_MASK  = 0x20000000;
    // A mask to get data format
    constexpr Type FORMAT_MASK  = 0x18000000;
    // A mask to get just the base type
    constexpr Type TYPE_MASK    = 0x07FFFFFF;

    inline bool is_digital(Type t) {
        return (t & DIGITAL_MASK) != 0; 
    }

    inline bool is_si(Type t) {
        return (t & DIGITAL_MASK) == 0; 
    }

    inline bool is_inverted(Type t) {
        return (t & INVERT_MASK) != 0; 
    }

    inline bool is_normal(Type t) {
        return (t & INVERT_MASK) == 0; 
    }

    inline bool get_format(Type t) {
        return (t & FORMAT_MASK) == 0; 
    }

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

    /**
     * @brief Builds a payload for a TEDS response.
     */
    inline std::vector<char> create_response_payload(Type type, float value) {
        struct ResponsePayload {
            Type type;
            float value;
        } __attribute__((packed));

        ResponsePayload payload{make_response_type(type), value};

        const char* start = reinterpret_cast<const char*>(&payload);
        const char* end = start + sizeof(ResponsePayload);
        return std::vector<char>(start, end);
    }
}

#endif // TEDS_HPP