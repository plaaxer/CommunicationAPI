#ifndef QUADRANT_HPP
#define QUADRANT_HPP

#include <cstdint>

enum class Quadrant : uint8_t {
    NORTH = 0, // 00
    EAST  = 1, // 01
    SOUTH = 2, // 10
    WEST  = 3  // 11
};

#endif // QUADRANT_HPP