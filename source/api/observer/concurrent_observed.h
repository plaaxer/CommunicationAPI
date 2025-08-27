#ifndef CONCURRENT_OBSERVED_H
#define CONCURRENT_OBSERVED_H

#include <list>       // For std::list
#include <algorithm>  // For std::find

// Forward-declare the Concurrent_Observer template
template<typename D, typename C>
class Concurrent_Observer;

/**
 * @brief The Observed (Non-Concurrent Version)
 * @tparam D Generic Data
 * @tparam C Generic Condition
 */
template<typename D, typename C = void>
class Concurrent_Observed
{
public:
    using Observer_Type = Concurrent_Observer<D, C>;
    using Observer_Pair = std::pair<C, Observer_Type*>;

    Concurrent_Observed() = default;
    ~Concurrent_Observed() = default;

    /**
     * @brief Add an Observer for a specific condition.
     * @param o Pointer to an Observer
     * @param c The condition to associate with the observer
    */
    void attach(Observer_Type* o, C c) {
        // Add the observer and its condition together as a pair to the list.
        _observers.push_back({c, o});
    }

    /**
     * @brief Removes an Observer for a specific condition.
     * @param o The Observer pointer to remove
     * @param c The associated condition
     */
    void detach(Observer_Type* o, C c) {
        // Remove the specific pair {condition, observer} from the list.
        _observers.remove({c, o});
    }

    /**
     * @brief Notifies each Observer subscribed to the given condition.
     * @param c The condition to notify
     * @param d Data pointer
     * @return True if at least one Observer was notified
     */
    bool notify(C c, D* d) {
        bool notified = false;
        // Iterate through the list of {condition, observer} pairs.
        for (const auto& pair : _observers) {
            // Check if the stored condition (pair.first) matches.
            if (pair.first == c) {
                // If it matches, notify the observer (pair.second).
                pair.second->update(this, c, d);
                notified = true;
            }
        }
        return notified;
    }

private:
    // A single list that stores pairs of {Condition, Observer*}.
    std::list<Observer_Pair> _observers;
};

#endif  // CONCURRENT_OBSERVED_H