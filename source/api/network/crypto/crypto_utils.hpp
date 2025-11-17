#ifndef CRYPTO_UTILS_HPP
#define CRYPTO_UTILS_HPP

#include <random>

#include "api/network/definitions/network_types.hpp"

class CryptoUtils
{

public:
    /**
     * @brief Generates a random 64-bit unsigned integer (thread safe)
     */
    static SessionKey generate_session_key()
    {
        // one engine/seed thread isolated
        thread_local static std::random_device rd;

        // Mersenne Twister engine
        thread_local static std::mt19937_64 engine(rd());

        std::uniform_int_distribution<uint64_t> distribution;

        return distribution(engine);
    }
};

#endif  // CRYPTO_UTILS_HPP

