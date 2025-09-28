#ifndef BUFFER_POOL_HPP
#define BUFFER_POOL_HPP

#include "api/network/definitions/buffer.hpp"
#include <vector>
#include <mutex>
#include <condition_variable>
#include <cstddef>

template<typename T>
class BufferPool {
private:
    // For convenience, create a using alias for the private ControlBlock struct.
    using ControlBlock = typename Buffer<T>::ControlBlock;

    // The pool now stores raw pointers to the memory blocks it owns.
    std::vector<ControlBlock*> _pool;
    std::mutex _mutex;
    std::condition_variable _condition;

    // This private method is the target for the Buffer's custom deleter.
    void release_block(ControlBlock* block) {
        std::lock_guard<std::mutex> lock(_mutex);
        _pool.push_back(block);
        // Notify one waiting thread that a buffer is now available.
        _condition.notify_one();
    }

public:
    /**
     * @brief Constructs the pool and pre-allocates a fixed number of buffers.
     */
    explicit BufferPool(size_t size) {
        for (size_t i = 0; i < size; ++i) {
            // Allocate raw memory for the control block and the data object.
            void* mem = ::operator new(sizeof(ControlBlock) + sizeof(T));
            ControlBlock* block = static_cast<ControlBlock*>(mem);
            
            // The deleter is a lambda function that captures 'this' pointer of the pool
            // and calls the private release_block method.
            block->deleter = [this](ControlBlock* b){ this->release_block(b); };
            
            // Use placement new to construct the data object in the allocated memory.
            new (block + 1) T();
            
            _pool.push_back(block);
        }
    }

    /**
     * @brief Destructor that cleans up all memory blocks owned by the pool.
     */
    ~BufferPool() {
        for (auto* block : _pool) {
            // Manually call the destructor for the data object.
            reinterpret_cast<T*>(block + 1)->~T();
            // Free the raw memory.
            ::operator delete(block);
        }
    }

    /**
     * @brief Acquires a Buffer handle from the pool.
     * If the pool is empty, this call will block until a buffer is released.
     */
    Buffer<T> acquire() {
        std::unique_lock<std::mutex> lock(_mutex);
        
        // Wait until the pool is not empty.
        _condition.wait(lock, [this]{ return !_pool.empty(); });
        
        ControlBlock* block = _pool.back();
        _pool.pop_back();
        
        block->ref_count = 1; // Set initial reference count for the new handle.
        
        // Return a Buffer handle that wraps the memory block.
        return Buffer<T>(block);
    }
};

#endif // BUFFER_POOL_HPP