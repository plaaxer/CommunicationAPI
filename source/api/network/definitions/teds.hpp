
#ifndef TEDS_HPP
#define TEDS_HPP

#include <cstdint>
#include <iostream>

#include "api/network/definitions/payload.hpp"

class TEDS {
    public:
        typedef uint16_t Type;

        typedef uint32_t Period;

        static const Type NO_TYPE = 0;

        Type extract_type(const void* data, unsigned int length) {
            std::cout << "WARNING: TEDS::extract_type() is not yet implemented and should not be used!" << std::endl;
            return -1;
            // todo: should extract the type from the smart data structure
        }
};

#endif // TEDS_HPP