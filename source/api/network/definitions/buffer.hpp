#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <cstddef>  // for size_t
#include <atomic>   // for std::atomic, for thread-safe reference counting
#include <utility>  // for std::move

template<typename T>
class Buffer {
private:
    // The Control Block holds metadata separate from the data itself.
    struct ControlBlock {
        std::atomic<int> ref_count; // The number of Buffer instances pointing to this block.
    };

    // Pointer to the start of the control block.
    // The actual data of type T is stored immediately after it in memory.
    ControlBlock* _block;

    // Private constructor forces users to create Buffers via the static alloc() method.
    explicit Buffer(ControlBlock* block) : _block(block) {}

public:
    /**
     * @brief Factory method to allocate memory for a new buffer.
     * @return A new Buffer instance managing the allocated memory.
     */
    static Buffer alloc() {
        // Allocate enough memory for our control block AND the actual data (T).
        void* mem = ::operator new(sizeof(ControlBlock) + sizeof(T));
        
        ControlBlock* block = static_cast<ControlBlock*>(mem);
        block->ref_count = 1; // Start with one reference.
        
        // Use placement new to construct the T object in the allocated memory.
        new (block + 1) T();

        return Buffer(block);
    }

    // Default constructor for an empty/invalid buffer.
    Buffer() : _block(nullptr) {}

    // Copy constructor: increments the reference count.
    Buffer(const Buffer& other) : _block(other._block) {
        if (_block) {
            _block->ref_count++;
        }
    }

    // Move constructor: "steals" the pointer from the other buffer.
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

    // Destructor: decrements the reference count and frees memory if it's the last reference.
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
            // Call the destructor for T before freeing the memory.
            data()->~T();
            ::operator delete(_block);
            _block = nullptr;
        }
    }
};

#endif // BUFFER_HPP




// template<typename T>
// class Buffer {
// private:
//     std::vector<T> buffer;
//     size_t head = 0;      // write index
//     size_t tail = 0;      // read index
//     size_t capacity;
//     bool full = false;

// public:
//     Buffer(size_t size) : buffer(size), capacity(size) {}
//     // To simulate push: Use write_ptr to get a pointer to write on, then advance_head
//     // To simulate pop: Use read_ptr to get the pointer to the data, then advance_tail

//     // Get pointer to next writable slot, or nullptr if full
//     T* write_ptr() {
//         if (full) return nullptr;
//         return &buffer[head];
//     }

//     // Advance head after writing
//     void advance_head() {
//         if (full) return;
//         head = (head + 1) % capacity;
//         full = (head == tail);
//     }

//     // Get pointer to next readable slot, or nullptr if empty
//     T* read_ptr() {
//         if (empty()) return nullptr;
//         return &buffer[tail];
//     }

//     // Advance tail after reading
//     void advance_tail() {
//         if (empty()) return;
//         full = false;
//         tail = (tail + 1) % capacity;
//     }

//     bool empty() const { return (!full && head == tail); }
//     bool is_full() const { return full; }
//     size_t size() const {
//         if (full) return capacity;
//         if (head >= tail) return head - tail;
//         return capacity + head - tail;
//     }
// };

