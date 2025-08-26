#ifndef BUFFER_HPP
#define BUFFER_HPP

template<typename T>
class Buffer {
private:
    std::vector<T> buffer;
    size_t head = 0;      // write index
    size_t tail = 0;      // read index
    size_t capacity;
    bool full = false;

public:
    Buffer(size_t size) : buffer(size), capacity(size) {}
    // To simulate push: Use write_ptr to get a pointer to write on, then advance_head
    // To simulate pop: Use read_ptr to get the pointer to the data, then advance_tail

    // Get pointer to next writable slot, or nullptr if full
    T* write_ptr() {
        if (full) return nullptr;
        return &buffer[head];
    }

    // Advance head after writing
    void advance_head() {
        if (full) return;
        head = (head + 1) % capacity;
        full = (head == tail);
    }

    // Get pointer to next readable slot, or nullptr if empty
    T* read_ptr() {
        if (empty()) return nullptr;
        return &buffer[tail];
    }

    // Advance tail after reading
    void advance_tail() {
        if (empty()) return;
        full = false;
        tail = (tail + 1) % capacity;
    }

    bool empty() const { return (!full && head == tail); }
    bool is_full() const { return full; }
    size_t size() const {
        if (full) return capacity;
        if (head >= tail) return head - tail;
        return capacity + head - tail;
    }
};

#endif