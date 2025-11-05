#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

#include <mutex>
#include <condition_variable>

class Semaphore {
public:
    explicit Semaphore(int initial_count = 0) : _count(initial_count) {}

    // 'v()' - Signal/Post/Increment
    void v() {
        std::lock_guard<std::mutex> lock(_mutex);
        _count++;
        _cv.notify_one();
    }

    // 'p()' - Wait/Decrement
    void p() {
        std::unique_lock<std::mutex> lock(_mutex);
        // Wait until the count is greater than 0.
        // The lambda prevents spurious wakeups.
        _cv.wait(lock, [&]{ return _count > 0; });
        _count--;
    }

    // Disable copying
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

private:
    std::mutex _mutex;
    std::condition_variable _cv;
    int _count;
};

#endif // SEMAPHORE_HPP