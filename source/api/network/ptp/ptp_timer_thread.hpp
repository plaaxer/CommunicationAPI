#ifndef PTP_TIMER_THREAD_HPP
#define PTP_TIMER_THREAD_HPP

#include <thread>
#include <atomic>
#include <chrono>

/**
 * @brief This class holds the thread that periodically triggers a PTP sync.
 *   It is designed to work with your SlaveSynchronizer.
*/
template <typename SlaveSynchronizer>
class PtpTimerThread {
public:
    PtpTimerThread() : _running(false) {}

    ~PtpTimerThread() {
        stop();
    }

    void start(SlaveSynchronizer* synchronizer, const Address& my_address) {

        _running = true;

        _thread = std::thread([this, synchronizer, my_address]() {

            std::this_thread::sleep_for(std::chrono::seconds(2));

            // by default we wait 2 seconds (for the rsu to start) and send a starter synchronization request, then entering the cycle
            synchronizer->request_synchronization(my_address);

            while (_running) {
                
                // for now let's just wait for 10 seconds until synchronizing.
                std::this_thread::sleep_for(std::chrono::seconds(10));
                
                if (!_running) break;

                std::cout << "[PTP Timer] Requesting clock synchronization..." << std::endl;

                synchronizer->request_synchronization(my_address);
            }
        });
    }

    void stop() {
        _running = false;
        if (_thread.joinable()) {
            _thread.join();
        }
    }

private:
    std::thread _thread;
    std::atomic<bool> _running;
};

#endif // PTP_TIMER_THREAD_HPP