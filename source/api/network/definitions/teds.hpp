
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
    * | bits 26 - 24 | Steradian (sr)   | (3 bits)  |
    * ----------------------------------------------- *
    * | bits 23 - 21 | Radian (r)       | (3 bits)  |
    * ----------------------------------------------- *
    * | bits 20 - 18 | Length (m)       | (3 bits)  | 
    * ----------------------------------------------- *
    * | bits 17 - 15 | Mass (kg)        | (3 bits)  |
    * ----------------------------------------------- *
    * | bits 14 - 12 | Time (s)         | (3 bits)  |
    * ----------------------------------------------- *
    * | bits 11 - 9  | Current (A)      | (3 bits)  |
    * ----------------------------------------------- *
    * | bits 8 - 6   | Temperature (K)  | (3 bits)  |
    * ----------------------------------------------- *
    * | bits 5 - 3   | Amount (mol)     | (3 bits)  |
    * ----------------------------------------------- *
    * | bits 2 - 0   | Light (cd)       | (3 bits)  |
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

    const uint32_t BASE = 0b00000100100100100100100100100100;
    const uint32_t CD  = 0b00000000000000000000000000000001; // cd
    const uint32_t MOL = 0b00000000000000000000000000001000; // m/s^2
    const uint32_t K   = 0b00000000000000000000000001000000; // K
    const uint32_t A   = 0b00000000000000000000001000000000; // A
    const uint32_t S   = 0b00000000000000000001000000000000; // s
    const uint32_t KG  = 0b00000000000000001000000000000000; // kg
    const uint32_t M   = 0b00000000000001000000000000000000; // m
    const uint32_t RAD = 0b00000000001000000000000000000000; // rad
    const uint32_t SR  = 0b00000001000000000000000000000000; // sr

    const uint32_t LUM_INTENSITY = BASE + CD;
    const uint32_t AMOUNT_OF_SUB = BASE + MOL;
    const uint32_t TEMPERATURE = BASE + K;
    const uint32_t CURRENT = BASE + A;
    const uint32_t TIME = BASE + S;
    const uint32_t MASS = BASE + KG;
    const uint32_t LENGTH = BASE + M;
    const uint32_t ANGLE = BASE + RAD;
    const uint32_t STERADIAN = BASE + SR;

    const uint32_t VELOCITY = BASE + M - S; // m/s
    const uint32_t ACCELERATION = BASE + M - 2*S; // m/s^2
    const uint32_t VOLTAGE = BASE + KG + 2*M - 3*S - A; // kg*m^2/(s^3*A)
    const uint32_t PRESSURE = BASE + KG - M - 2*S; // kg/(m*s^2)
    const uint32_t FREQUENCY = BASE - S; // 1/s
    const uint32_t LUMINANCE = BASE + CD - 2*M; // cd/m^2
    const uint32_t DENSITY = BASE + KG - 3*M; // kg/m^3 (Humidity is a type of density)
    const uint32_t FORCE = BASE + KG + M - 2*S; // kg*m/s^2
    const uint32_t FARAD = BASE - KG - 2*M + 4*S + 2*A; // s^4*A^2/(kg*m^2) (This unit is pushing the limits of our 32-bit format)

}

#endif // TEDS_HPP