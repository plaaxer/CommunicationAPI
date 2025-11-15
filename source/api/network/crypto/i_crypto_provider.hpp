#ifndef I_CRYPTO_PROVIDER_HPP
#define I_CRYPTO_PROVIDER_HPP

#include <vector>
#include <stdexcept>
#include <cstdint>


using SessionKey = uint64_t;
using ByteVector = std::vector<char>;
/**
 * @interface ICryptoProvider
 * @brief Abstract service interface for crypto services such as encrypting and decrypting.
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
    virtual ByteVector encrypt(const ByteVector& plaintext, const SessionKey& key) = 0;

    /**
     * @brief Decrypts a byte vector using a given session key.
     * @throws std::runtime_error if decrypting fails.
     */
    virtual ByteVector decrypt(const ByteVector& ciphertext, const SessionKey& key) = 0;
};

#endif // I_CRYPTO_PROVIDER_HPP