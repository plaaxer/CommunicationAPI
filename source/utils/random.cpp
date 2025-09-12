#include <random>
#include <chrono>

/**
 * @brief returns a random value between min and max
 */
inline int random_between(int min, int max) {
    static thread_local std::mt19937 gen(
        std::chrono::steady_clock::now().time_since_epoch().count()
    );
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}
