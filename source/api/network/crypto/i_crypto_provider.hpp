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
 * todo: implement services such as a simple xor and more advanced algorithms.
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