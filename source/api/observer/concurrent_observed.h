#ifndef CONCURRENT_OBSERVED_H
#define CONCURRENT_OBSERVED_H

/**
 * @brief The Observed
 * @tparam D Generic Data
 * @tparam C Generic Condition (void is the default type)
 */
template<typename D, typename C = void>
class Concurrent_Observed
{
    friend class Concurrent_Observer<D, C>;
public:
    typedef D Observed_Data;
    typedef C Observing_Condition;
    typedef Ordered_List<Concurrent_Observer<D, C>, C> Observers;

public:
    Concurrent_Observed() {}
    ~Concurrent_Observed() {}

    /**
     * @brief Add an Observer to the list of observers
     * @param o Pointer for a Observer
     * @param c Associated condition
    */
    void attach(Concurrent_Observer<D, C> * o, C c) {
        _observers.insert(o);
    }

    /**
     * @brief Removes an Observer from the list of observers
     * @param o Observer pointer
     * @param c Associated condition
     */
    void detach(Concurrent_Observer<D, C> * o, C c) {
        _observers.remove(o);
    }

    /**
     * @brief Notifies each Observer who is subscribed and respects the condition
     * @param c Condition associated
     * @param d Data pointer
     * @return Indicates if at least one Observer was notified
     */
    bool notify(C c, D * d) {
        bool notified = false;
        for (Observers::Iterator obs = _observers.begin(); obs != _observers.end(); obs++) {
            // IT IS AN EXAMPLE! THERE IS NO CODE IN THE API FOR rank()
            if (obs->rank() == c) {
                obs->update(c, d);
                notified = true;
            }
        }
        return notified;
    }

private:
    Observers _observers;
};

#endif  // CONCURRENT_OBSERVED_H