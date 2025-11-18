#ifndef XOR_CRYPTO_PROVIDER_HPP
#define XOR_CRYPTO_PROVIDER_HPP

#include "api/network/crypto/i_crypto_provider.hpp"
#include <cstring> // For std::memcpy

/**
 * @class XorCryptoProvider
 * @brief Simple implementation of ICryptoProvider using XOR
 * with a uint64_t session key.
 */
class XorCryptoProvider : public ICryptoProvider {
public:

        /**
     * @brief Generates a simple 64-bit MAC using a running XOR checksum.
     * WARNING: This is NOT cryptographically secure, per the deliverable.
     *
     * @param data A pointer to the data to authenticate.
     * @param size The size of the data in bytes.
     * @param key The 64-bit session key.
     * @return A 64-bit MAC.
     */
    uint64_t generate_mac(const void* data, size_t size, uint64_t key) {
        // Start with the key as the initial MAC value
        uint64_t mac = key;
        
        // Cast the data to be able to access it in 8-byte chunks
        const uint64_t* data_as_u64 = static_cast<const uint64_t*>(data);
        
        // Process all full 8-byte chunks
        size_t i = 0;
        for (i = 0; i < size / 8; ++i) {
            mac ^= data_as_u64[i];
        }

        // Process any remaining bytes one by one
        const unsigned char* remaining_bytes = static_cast<const unsigned char*>(data) + (i * 8);
        for (size_t j = 0; j < size % 8; ++j) {
            // Fold the remaining byte into the MAC
            mac ^= static_cast<uint64_t>(remaining_bytes[j]) << (j * 8);
        }

        return mac;
    }

    /**
     * @brief Encrypts data using a byte-wise XOR with the 8 bytes
     * of the uint64_t session key.
     *
     * @param plaintext The data to encrypt.
     * @param key The 64-bit (8-byte) session key.
     * @return A new ByteVector containing the encrypted data.
     */

    /**
     * ATTENTION: this creates a copy of the encrypted data (or decrypted data if decrypting), thus cannot be used just anywhere.
     * Should ideally be used at the communicator to both decrypt the data and place it on the message at the same time.
     * Maybe this implementation should be changed to modify the plaintext itself then?
     */

    ByteVector encrypt(const ByteVector& plaintext, SessionKey key) override {
        if (key == 0) {
            // Using a key of 0 would result in no encryption.
            // Depending on policy, you could throw or just return the plaintext.
            // Let's assume for now it's an error.
            throw std::runtime_error("XOR session key cannot be zero.");
        }

        // get the 8 bytes of the key in a standard order
        unsigned char key_bytes[8];
        std::memcpy(key_bytes, &key, sizeof(key));

        // create a copy of the data to modify
        ByteVector ciphertext = plaintext;

        // XOR each byte of the data, cycling through the 8 key bytes
        for (size_t i = 0; i < ciphertext.size(); ++i) {
            // The key byte to use is i % 8
            ciphertext[i] = ciphertext[i] ^ key_bytes[i % 8];
        }
        
        return ciphertext;
    }

    /**
     * @brief Decrypts data using XOR, which is the same as encrypting.
     */
    ByteVector decrypt(const ByteVector& ciphertext, SessionKey key) override {
        return encrypt(ciphertext, key);
    }
};

#endif // XOR_CRYPTO_PROVIDER_HPP