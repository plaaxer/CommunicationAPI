#ifndef CONCURRENT_OBSERVER_H
#define CONCURRENT_OBSERVER_H

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
        _data.insert(d);
        _semaphore.v();  // Increments semaphore (v comes from Verhoog in german, means increment)
    }

    D * updated() {
        _semaphore.p();  // Decrements semaphore (p comes from Probeer in german, means try/consume)
        return _data.remove();
    }

private:
    Semaphore _semaphore;
    List<D> _data;
};

#endif  // CONCURRENT_OBSERVER_H