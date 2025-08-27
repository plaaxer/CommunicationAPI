#ifndef CONCURRENT_OBSERVER_H
#define CONCURRENT_OBSERVER_H

#include "api/observer/semaphore.hpp"
#include "api/observer/conditional_data_observer.hpp"
#include <list>

// Forward-declare the class template that this observer is attached to.
template<typename T, typename Condition>
class Conditionally_Data_Observed;

/**
 * @brief The Concurrent_Observer, now a specialized type of Conditional_Data_Observer.
 */
template<typename D, typename C = void>
class Concurrent_Observer : public Conditional_Data_Observer<D, C> // <-- 2. Inherit publicly
{
public:
    typedef D Observed_Data;
    typedef C Observing_Condition;

public:
    Concurrent_Observer() : _semaphore(0) {}
    ~Concurrent_Observer() {}

    /**
     * @brief This is the implementation of the pure virtual function from the base class.
     *
     * This method is called by the Protocol/Channel. It fulfills the
     * Conditional_Data_Observer interface.
     */
    void update(Conditionally_Data_Observed<D, C>* obs, C c, D* d) override {
        // Internally, it just calls our own queueing logic.
        this->update_internal(c, d);
    }

    /**
     * @brief Blocks until data is available, then returns it.
     */
    D* updated() {
        _semaphore.p();
        D* data_ptr = _data.front();
        _data.pop_front();
        return data_ptr;
    }

private:
    /**
     * @brief The original update logic that adds data to the queue.
     *
     * Renamed to avoid confusion with the virtual 'update' method.
     */
    void update_internal(C c, D* d) {


        _data.push_back(d);
        _semaphore.v();
    }

private:
    Semaphore _semaphore;
    std::list<D*> _data;
};

#endif  // CONCURRENT_OBSERVER_H