#ifndef CRYPTO_SERVICE_HPP
#define CRYPTO_SERVICE_HPP

#include "i_crypto_provider.hpp"
#include <memory>

/**
 * @class CryptoService
 * @brief A static service layer to access the active crypto provider.
 *
 * This class provides a static interface for cryptographic functions.
 * You MUST initialize it with a provider (e.g., XorCryptoProvider)
 * by calling CryptoService::init() at the start of your application.
 */
class CryptoService {
public:
    // Deleted constructor - this is a static-only class
    CryptoService() = delete;

    /**
     * @brief Initializes the service with a specific crypto provider.
     * This MUST be called once before using any other functions.
     * @param provider A unique_ptr to an implementation of ICryptoProvider.
     */
    static void init(std::unique_ptr<ICryptoProvider> provider);

    /**
     * @brief Encrypts data using the configured provider.
     */
    static ByteVector encrypt(const ByteVector& plaintext, SessionKey key);

    /**
     * @brief Decrypts data using the configured provider.
     */
    static ByteVector decrypt(const ByteVector& ciphertext, SessionKey key);

    /**
     * @brief Generates a MAC using the configured provider.
     */
    static uint64_t generate_mac(const void* data, size_t size, SessionKey key);

private:
    // Helper to check if the provider is initialized
    static ICryptoProvider* getProvider();

    // The static instance of the provider
    static std::unique_ptr<ICryptoProvider> s_provider;
};

#endif // CRYPTO_SERVICE_HPP