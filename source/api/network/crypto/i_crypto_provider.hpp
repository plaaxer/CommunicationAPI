#ifndef I_CRYPTO_PROVIDER_HPP
#define I_CRYPTO_PROVIDER_HPP

#include <vector>
#include <stdexcept>
#include <cstdint>


using SessionKey = uint64_t;
using MsgAuthCode = uint64_t;
using ByteVector = std::vector<char>;
/**
 * @interface ICryptoProvider
 * @brief Abstract service interface for crypto services such as encrypting decrypting, and mac generation.
 * @details Todo: implement more advanced algorithms.
 * Beware that currently the session key is only 64 bits long; that's fine for regular XOR,
 * but for stuff like AES that should be changed. If so, the atomicity at the shared memory
 * engine variable "session_key" should be reviewed.
 */
class ICryptoProvider {
public:
    virtual ~ICryptoProvider() = default;

    /**
     * @brief Encrypts a byte vector with a given session key.
     */
    virtual ByteVector encrypt(const ByteVector& plaintext, SessionKey key) = 0;

    /**
     * @brief Decrypts a byte vector using a given session key.
     * @throws std::runtime_error if decrypting fails.
     */
    virtual ByteVector decrypt(const ByteVector& ciphertext, SessionKey key) = 0;

    /**
     * @brief Generates a mac using the selected provider.
     * @throws std::runtime_error if the generation is not implemented.
     */
    virtual uint64_t generate_mac(const void* data, size_t size, SessionKey key) {
        throw std::runtime_error("Generate mac not implemented for the selected provider!");
    }
};

#endif // I_CRYPTO_PROVIDER_HPP