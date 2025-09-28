#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <atomic>
#include <utility>
#include <functional> // Required for std::function

// Forward-declare the BufferPool so Buffer can befriend it.
template<typename T>
class BufferPool;

template<typename T>
class Buffer {
private:
    // The Control Block now includes a function pointer for the custom deleter.
    struct ControlBlock {
        std::atomic<int> ref_count;
        std::function<void(ControlBlock*)> deleter;
    };

    ControlBlock* _block;

    // The constructor is private, ensuring only the friend class BufferPool can create
    // Buffer handles from raw memory blocks.
    explicit Buffer(ControlBlock* block) : _block(block) {}

    // The BufferPool is a friend, allowing it to access the private constructor.
    friend class BufferPool<T>;

public:
    // Default constructor creates an empty/invalid buffer.
    Buffer() : _block(nullptr) {}

    // Copy constructor: increments the reference count.
    Buffer(const Buffer& other) : _block(other._block) {
        if (_block) {
            _block->ref_count++;
        }
    }

    // Move constructor: steals the pointer from the other buffer.
    Buffer(Buffer&& other) noexcept : _block(other._block) {
        other._block = nullptr;
    }

    // Assignment operator: handles self-assignment and reference counting.
    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            release(); // Release the current resource.
            _block = other._block;
            if (_block) {
                _block->ref_count++;
            }
        }
        return *this;
    }
    
    // Move assignment operator.
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            release();
            _block = other._block;
            other._block = nullptr;
        }
        return *this;
    }

    // Destructor: decrements the reference count.
    ~Buffer() {
        release();
    }

    // Provides access to the structured data (e.g., Ethernet::Frame).
    T* data() const {
        if (!_block) return nullptr;
        // The data is located right after the ControlBlock in memory.
        return reinterpret_cast<T*>(_block + 1);
    }

    // Check if the buffer is valid.
    explicit operator bool() const {
        return _block != nullptr;
    }

private:
    // Helper function to release the managed resource.
    void release() {
        if (_block && --_block->ref_count == 0) {
            // CRITICAL CHANGE: Instead of deleting memory, we call our custom deleter
            // which will return the memory block to the BufferPool.
            if (_block->deleter) {
                _block->deleter(_block);
            }
            _block = nullptr;
        }
    }
};

#endif // BUFFER_HPP