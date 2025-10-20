
#ifndef TEDS_HPP
#define TEDS_HPP

#include <cstdint>
#include <iostream>

#include "api/network/definitions/segment.hpp"

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
    using Period = unsigned int;

    constexpr Type INVALID = 0b00000000000000000000000000000000;

    // A mask to isolate the MSB (the "purpose" bit)
    constexpr Type PURPOSE_MASK = 0x80000000;
    // A mask to get digital/SI type
    constexpr Type DIGITAL_MASK = 0x40000000;
    // A mask to get if data is inverted
    constexpr Type INVERT_MASK  = 0x20000000;
    // A mask to get data format
    constexpr Type FORMAT_MASK  = 0x18000000; // 00011000000000...
    // A mask to get just the base type
    constexpr Type TYPE_MASK    = 0x07FFFFFF;

    // Used to determine what format of data is being used in get_format
    // enum class DataFormat : uint32_t {
    //     INT = 0x10000000;
    //     FLOAT = 0x18000000;
    // }
    constexpr Type FORMAT_INT   = 0x00000000; 
    constexpr Type FORMAT_FLOAT = 0x08000000; // 0000 1000 0000..

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
        return (t & FORMAT_MASK); 
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


    inline Type extract_type(const Segment::Header* seg_header, const void* seg_payload, unsigned int payload_size) {

        if (seg_header->type != Segment::MsgType::TEDS) {
            return INVALID;
        }
        if (payload_size < sizeof(TEDS::Header)) {
            return INVALID;
        }
        const TEDS::Header* teds_header = static_cast<const TEDS::Header*>(seg_payload);
        return teds_header->type;
    }

    constexpr Type BASE = 0b00000100100100100100100100100100;
    constexpr Type CD  = 0b00000000000000000000000000000001; // cd
    constexpr Type MOL = 0b00000000000000000000000000001000; // m/s^2
    constexpr Type K   = 0b00000000000000000000000001000000; // K
    constexpr Type A   = 0b00000000000000000000001000000000; // A
    constexpr Type S   = 0b00000000000000000001000000000000; // s
    constexpr Type KG  = 0b00000000000000001000000000000000; // kg
    constexpr Type M   = 0b00000000000001000000000000000000; // m
    constexpr Type RAD = 0b00000000001000000000000000000000; // rad
    constexpr Type SR  = 0b00000001000000000000000000000000; // sr

    constexpr Type LUM_INTENSITY = BASE + CD;
    constexpr Type AMOUNT_OF_SUB = BASE + MOL;
    constexpr Type TEMPERATURE = BASE + K;
    constexpr Type CURRENT = BASE + A;
    constexpr Type TIME = BASE + S;
    constexpr Type MASS = BASE + KG;
    constexpr Type LENGTH = BASE + M;
    constexpr Type ANGLE = BASE + RAD;
    constexpr Type STERADIAN = BASE + SR;

    constexpr Type VELOCITY = BASE + M - S; // m/s
    constexpr Type ACCELERATION = BASE + M - 2*S; // m/s^2
    constexpr Type VOLTAGE = BASE + KG + 2*M - 3*S - A; // kg*m^2/(s^3*A)
    constexpr Type PRESSURE = BASE + KG - M - 2*S; // kg/(m*s^2)
    constexpr Type FREQUENCY = BASE - S; // 1/s
    constexpr Type LUMINANCE = BASE + CD - 2*M; // cd/m^2
    constexpr Type DENSITY = BASE + KG - 3*M; // kg/m^3 (Humidity is a type of density)
    constexpr Type FORCE = BASE + KG + M - 2*S; // kg*m/s^2
    constexpr Type FARAD = BASE - KG - 2*M + 4*S + 2*A; // s^4*A^2/(kg*m^2) (This unit is pushing the limits of our 32-bit format)


    inline std::string get_type_name(Type full_type) {
        // First, strip all metadata bits to get the pure physical unit identifier.
        Type base_type = get_base_type(full_type);

        switch (base_type) {
            // Derived SI Units
            case LUM_INTENSITY:   return "Luminous Intensity";
            case AMOUNT_OF_SUB:   return "Amount of Substance";
            case TEMPERATURE:     return "Temperature";
            case CURRENT:         return "Current";
            case TIME:            return "Time";
            case MASS:            return "Mass";
            case LENGTH:          return "Length";
            case ANGLE:           return "Angle";
            case STERADIAN:       return "Solid Angle (Steradian)";
            case VELOCITY:        return "Velocity";
            case ACCELERATION:    return "Acceleration";
            case VOLTAGE:         return "Voltage";
            case PRESSURE:        return "Pressure";
            case FREQUENCY:       return "Frequency";
            case LUMINANCE:       return "Luminance";
            case DENSITY:         return "Density";
            case FORCE:           return "Force";
            case FARAD:           return "Capacitance (Farad)";

            // Base SI Units (in case they are used directly)
            case CD:              return "Luminous Intensity (base)";
            case MOL:             return "Amount of Substance (base)";
            case K:               return "Temperature (base)";
            case A:               return "Current (base)";
            case S:               return "Time (base)";
            case KG:              return "Mass (base)";
            case M:               return "Length (base)";
            case RAD:             return "Angle (base)";
            case SR:              return "Solid Angle (base)";
            
            // Default case for any unknown or invalid types
            default:
                return "Unknown Type";
        }
    }

}

#endif // TEDS_HPP