#include "crypto_service.hpp"
#include <stdexcept>

std::unique_ptr<ICryptoProvider> CryptoService::s_provider = nullptr;

// gets the provider
ICryptoProvider* CryptoService::getProvider() {
    if (!s_provider) {
        throw std::runtime_error("CryptoService not initialized. Call CryptoService::init() first.");
    }
    return s_provider.get();
}

void CryptoService::init(std::unique_ptr<ICryptoProvider> provider) {
    s_provider = std::move(provider);
}

ByteVector CryptoService::encrypt(const ByteVector& plaintext, SessionKey key) {
    return getProvider()->encrypt(plaintext, key);
}

ByteVector CryptoService::decrypt(const ByteVector& ciphertext, SessionKey key) {
    return getProvider()->decrypt(ciphertext, key);
}

uint64_t CryptoService::generate_mac(const void* data, size_t size, SessionKey key) {
    return getProvider()->generate_mac(data, size, key);
}