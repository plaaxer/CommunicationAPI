#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

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
};

#endif // CLOCK_HPP
