#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <cstdio>

class Clock {
public:
    // Returns the current system time as a formatted string, e.g. "2025-10-28 19:45:12"
    static std::string getCurrentTimeString() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);

        std::tm localTime{};
        localtime_r(&t, &localTime);  // Thread-safe version for Linux

        std::ostringstream oss;
        oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    // Returns the current time in milliseconds since Unix epoch
    static long long getCurrentTimeMillis() {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        );
        return ms.count();
    }

    // Returns the current time in seconds since Unix epoch
    static long long getCurrentTimeSeconds() {
        auto now = std::chrono::system_clock::now();
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()
        );
        return sec.count();
    }

    /**
     * @brief Abruptly "jumps" the VM's system clock by a specified offset.
     * WARNING: Requires root privileges.
     *
     * @param offset_ms The time offset to apply, in MILLISECONDS.
     * @return true on success, false on failure.
     */
    static bool setClockOffset(int64_t offset_ms) {
        // 1. Get the current local time
        struct timeval current_time;
        if (gettimeofday(&current_time, nullptr) != 0) {
            perror("gettimeofday failed");
            return false;
        }

        // 2. Convert the millisecond offset to seconds and microseconds
        int64_t offset_sec = offset_ms / 1000;
        int64_t offset_usec = (offset_ms % 1000) * 1000;

        // 3. Calculate the new, corrected time
        struct timeval new_time;
        new_time.tv_sec = current_time.tv_sec + offset_sec;
        new_time.tv_usec = current_time.tv_usec + offset_usec;

        // 4. Handle rollover in microseconds
        if (new_time.tv_usec >= 1000000) {
            new_time.tv_sec++;
            new_time.tv_usec -= 1000000;
        } else if (new_time.tv_usec < 0) {
            new_time.tv_sec--;
            new_time.tv_usec += 1000000;
        }

        // 5. Set the system clock to the new time
        if (settimeofday(&new_time, nullptr) != 0) {
            perror("settimeofday failed (are you root?)");
            return false;
        }
        return true;
    }
};

#endif // CLOCK_HPP
