#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <cstddef>
#include <atomic>
#include <utility>
#include <cstdint> // For uint64_t

template<typename T>
class Buffer {
private:
    // For owning buffers: holds metadata separate from the data.
    struct ControlBlock {
        std::atomic<int> ref_count;
    };

    // --- MEMBER VARIABLES ---
    // Used for owning, reference-counted buffers. Null for views.
    ControlBlock* _block; 
    
    // Used for non-owning views. Null for owning buffers.
    T* _view_ptr;

    // Distinguishes between the two modes.
    bool _is_view;

    // Stores the sequence ID, used only in view mode for releasing the SHM slot.
    uint64_t _sequence_id;

    // Private constructor for the alloc() factory method.
    explicit Buffer(ControlBlock* block) 
        : _block(block), _view_ptr(nullptr), _is_view(false), _sequence_id(0) {}

public:
    /**
     * @brief Factory method for a standard, memory-owning buffer.
     */
    static Buffer alloc() {
        void* mem = ::operator new(sizeof(ControlBlock) + sizeof(T));
        ControlBlock* block = static_cast<ControlBlock*>(mem);
        block->ref_count = 1;
        new (block + 1) T(); // Placement new for the data object
        return Buffer(block);
    }

    /**
     * @brief **NEW:** Public constructor for creating a non-owning "view".
     * @param data_ptr A raw pointer to existing data (e.g., in shared memory).
     * @param seq_id The sequence ID associated with the data, for later release.
     */
    Buffer(T* data_ptr, uint64_t seq_id) 
        : _block(nullptr), _view_ptr(data_ptr), _is_view(true), _sequence_id(seq_id) {}

    // Default constructor for an empty/invalid buffer.
    Buffer() : _block(nullptr), _view_ptr(nullptr), _is_view(false), _sequence_id(0) {}

    // Destructor: properly handles both modes.
    ~Buffer() {
        // For owning buffers, release() handles reference counting and deallocation.
        // For views, _block is null, so release() does nothing, which is correct.
        release();
    }

    // Copy constructor.
    Buffer(const Buffer& other) {
        _is_view = other._is_view;
        _sequence_id = other._sequence_id;
        _view_ptr = other._view_ptr;
        _block = other._block;
        if (!_is_view && _block) {
            _block->ref_count++;
        }
    }

    // Move constructor.
    Buffer(Buffer&& other) noexcept {
        _is_view = other._is_view;
        _sequence_id = other._sequence_id;
        _view_ptr = other._view_ptr;
        _block = other._block;

        // Invalidate the other buffer
        other._block = nullptr;
        other._view_ptr = nullptr;
    }

    // Copy assignment operator.
    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            release(); // Release current resource
            _is_view = other._is_view;
            _sequence_id = other._sequence_id;
            _view_ptr = other._view_ptr;
            _block = other._block;
            if (!_is_view && _block) {
                _block->ref_count++;
            }
        }
        return *this;
    }
    
    // Move assignment operator.
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            release();
            _is_view = other._is_view;
            _sequence_id = other._sequence_id;
            _view_ptr = other._view_ptr;
            _block = other._block;
            
            other._block = nullptr;
            other._view_ptr = nullptr;
        }
        return *this;
    }
    
    /**
     * @brief Provides access to the data object (T).
     * @return A pointer to the data, either in managed memory or in the viewed location.
     */
    T* data() const {
        if (_is_view) {
            return _view_ptr;
        }
        if (!_block) return nullptr;
        return reinterpret_cast<T*>(_block + 1);
    }

    bool is_view() const { return _is_view; }
    uint64_t sequence_id() const { return _sequence_id; }

    // Check if the buffer is valid (points to something).
    explicit operator bool() const {
        return _block != nullptr || _view_ptr != nullptr;
    }

private:
    // Helper to release the managed resource (for owning buffers only).
    void release() {
        if (_block && --_block->ref_count == 0) {
            data()->~T(); // Explicitly call destructor for the data object
            ::operator delete(_block);
            _block = nullptr;
        }
    }
};

#endif // BUFFER_HPP