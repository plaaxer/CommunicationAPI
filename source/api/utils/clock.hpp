#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <cstdio>
#include <random>
#include <iostream>

class Clock 
{
public:

    /**
     * @brief Desynchronizes the system clock to test the PTP implementation
     */
    static void desynchronize()
    {
        std::random_device rd;  // harware based seed
        
        std::mt19937 gen(rd());  // mersenne twister

        // 5 to 20 seconds (in mili)
        std::uniform_int_distribution<int64_t> offset_dist(5000, 20000);
        
        // direction (0 = negative, 1 = positive)
        std::uniform_int_distribution<> direction_dist(0, 1);

        // base offset
        int64_t random_offset_ms = offset_dist(gen);
        
        // randomly choose if it will be negative or positive
        if (direction_dist(gen) == 0) {
            random_offset_ms = -random_offset_ms;
        }

        int64_t current = Clock::getCurrentTimeMillis();
        int64_t new_current = current - random_offset_ms;

        // std::cout << "[Clock] System will be desynchronized by " << random_offset_ms << "ms" << std::endl;

        if (setClockAbruptly(new_current)) {
            std::cout << "[Clock] System time after desync:  " << getCurrentTimeString() << std::endl;
        } else {
            std::cerr << "\n[Error] Failed to set clock offset." << std::endl;
        }
    }
        
    // Returns the current system time as a formatted string, e.g. "2025-10-28 19:45:12", but w/ ms
    static std::string getCurrentTimeString()
    {
        auto now = std::chrono::system_clock::now();

        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
        localtime_r(&t, &localTime); // thread-safe

        // the fractional part (milliseconds)
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        );
        
        // get just the part after the whole second
        int64_t fractional_ms = duration_ms.count() % 1000; 

        std::ostringstream oss;
        oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S"); // format Y-m-d H:M:S
        oss << "." << std::setw(3) << std::setfill('0') << fractional_ms; // format .ms

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

    // Returns the current time in nanoseconds since Unix epoch
    static long long getCurrentTimeNanos() {
        auto now = std::chrono::system_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()
        );
    return ns.count();
}

    /**
     * @brief Formats a millisecond-since-epoch timestamp (as a uint64_t) into a readable string.
     * @param timestamp_ms The timestamp in milliseconds since Unix epoch.
     * @return A formatted string, e.g., "2025-10-28 19:45:12.123"
     */
    static std::string getFormattedTimestamp(uint64_t timestamp_ms) {
        // 1. Convert the millisecond count into a std::chrono time_point
        std::chrono::milliseconds duration_ms(timestamp_ms);
        std::chrono::system_clock::time_point time_point(duration_ms);

        // 2. Get the time_t (seconds) component for the main formatting
        std::time_t t = std::chrono::system_clock::to_time_t(time_point);

        // 3. Get the calendar time structure (thread-safe)
        std::tm localTime{};
        localtime_r(&t, &localTime);

        // 4. Calculate the leftover millisecond part (safe for uint64_t)
        uint64_t just_ms = timestamp_ms % 1000;

        // 5. Format the string
        std::ostringstream oss;
        oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
        oss << "." << std::setw(3) << std::setfill('0') << just_ms;
        
        return oss.str();
    }

    static bool setClockAbruptly(uint64_t new_time_ms) {
        // 1. Convert the millisecond timestamp to seconds and microseconds
        uint64_t sec = new_time_ms / 1000;
        uint64_t usec = (new_time_ms % 1000) * 1000;

        // 2. Create the timeval struct for the new time
        struct timeval new_time;
        new_time.tv_sec = sec;
        new_time.tv_usec = usec;

        // 3. Set the system clock to the new absolute time
        if (settimeofday(&new_time, nullptr) != 0) {
            perror("settimeofday failed (are you root?)");
            return false;
        }
        return true;
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

        // printf("[Clock] offset: %ld \n", offset_ms);

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

        // printf("[DEBUG] New time (final): %ld sec, %ld usec\n", new_time.tv_sec, new_time.tv_usec);

        // 5. Set the system clock to the new time
        if (settimeofday(&new_time, nullptr) != 0) {
            perror("settimeofday failed (are you root?)");
            return false;
        }
        return true;
    }
};

#endif // CLOCK_HPP
