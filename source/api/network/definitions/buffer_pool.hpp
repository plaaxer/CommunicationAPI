#ifndef BUFFER_POOL_HPP
#define BUFFER_POOL_HPP

#include "api/network/definitions/buffer.hpp"
#include <vector>
#include <mutex>
#include <condition_variable>

template<typename T>
class BufferPool {
public:
    /**
     * @brief Constructs the pool and pre-allocates a fixed number of buffers.
     * @param size The number of buffers to create in the pool.
     */
    BufferPool(size_t size) {
        // Pre-allocate all the buffers the system will use.
        for (size_t i = 0; i < size; ++i) {
            _pool.push_back(Buffer<T>::alloc());
        }
    }

    /**
     * @brief Acquires a buffer from the pool.
     * If the pool is empty, this call will block until a buffer is released.
     * @return A Buffer object.
     */
    Buffer<T> acquire() {
        std::unique_lock<std::mutex> lock(_mutex);
        
        // Wait for the condition that the pool is not empty.
        _condition.wait(lock, [this]{ return !_pool.empty(); });
        
        // Once woken up, take a buffer from the back.
        Buffer<T> buf = std::move(_pool.back());
        _pool.pop_back();
        
        return buf;
    }

    /**
     * @brief Returns a buffer to the pool for reuse.
     * @param buf The Buffer object to release.
     */
    void release(Buffer<T>&& buf) {
        std::lock_guard<std::mutex> lock(_mutex);
        _pool.push_back(std::move(buf));
        
        // Notify one waiting thread that a buffer is now available.
        _condition.notify_one();
    }

private:
    std::vector<Buffer<T>> _pool;
    std::mutex _mutex;
    std::condition_variable _condition;
};

#endif // BUFFER_POOL_HPP