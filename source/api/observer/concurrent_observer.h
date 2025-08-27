#ifndef CONCURRENT_OBSERVER_H
#define CONCURRENT_OBSERVER_H

#include "api/observer/semaphore.hpp"

#include <list> // Standard C++ list

template<typename D, typename C>
class Concurrent_Observed;

/**
 * @brief The Observer
 * @tparam D Generic Data
 * @tparam C Generic Condition of this Observer/Subscriber
 */
template<typename D, typename C = void>
class Concurrent_Observer
{
    friend class Concurrent_Observed<D, C>;
public:
    typedef D Observed_Data;
    typedef C Observing_Condition;

public:
    Concurrent_Observer() : _semaphore(0) {}
    ~Concurrent_Observer() {}

    /**
     * @brief Method where the Observed/Publisher will update the Observer/Subscriber 
     * @param c Condition
     * @param d Data pointer
     */
    void update(C c, D * d) {
        // FIX: Use push_back() to add the item to the end of the std::list.
        _data.push_back(d);
        _semaphore.v();  // Increments semaphore
    }

    /**
     * @brief Blocks until data is available, then returns it.
     * @return A pointer to the observed data.
     */
    D * updated() {
        _semaphore.p();  // Decrements semaphore, blocking if it's zero.
        
        D* data_ptr = _data.front(); // 1. Get the first item.
        _data.pop_front();           // 2. Remove it from the list.
        return data_ptr;             // 3. Return the item.
    }

private:
    Semaphore _semaphore;
    std::list<D*> _data;
};

#endif  // CONCURRENT_OBSERVER_H