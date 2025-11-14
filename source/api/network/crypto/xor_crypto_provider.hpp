#ifndef XOR_CRYPTO_PROVIDER_HPP
#define XOR_CRYPTO_PROVIDER_HPP

#include "api/network/crypto/i_crypto_provider.hpp"

/**
 * @class XorCryptoProvider
 * @brief Simple implementation of ICryptoProvider using XOR.
 */
class XorCryptoProvider : public ICryptoProvider {
public:
    /**
     * ATTENTION: this creates a copy of the encrypted data (or decrypted data if decrypting), thus cannot be used just anywhere.
     * Should ideally be used at the communicator to both decrypt the data and place it on the message at the same time.
     * Maybe this implementation should be changed to modify the plaintext itself then?
     */
    ByteVector encrypt(const ByteVector& plaintext, const SessionKey& key) override {
        if (key.empty()) {
            throw std::runtime_error("Empty session key when decrypting");
        }

        ByteVector ciphertext = plaintext; // copy
        for (size_t i = 0; i < ciphertext.size(); ++i) {
            // XORS each byte, repeating the key in case it is smaller
            ciphertext[i] = ciphertext[i] ^ key[i % key.size()];
        }
        return ciphertext;
    }

    ByteVector decrypt(const ByteVector& ciphertext, const SessionKey& key) override {
        return encrypt(ciphertext, key);
    }
};

#endif // XOR_CRYPTO_PROVIDER_HPP