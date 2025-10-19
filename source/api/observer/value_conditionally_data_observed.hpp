#ifndef VALUE_CONDITIONALLY_DATA_OBSERVED_H
#define VALUE_CONDITIONALLY_DATA_OBSERVED_H

#include <vector>
#include "api/observer/value_conditional_data_observer.hpp"

/**
 * @brief A subject that notifies observers with data by VALUE (copy).
 */
template <typename T, typename Condition = void>
class Value_Conditionally_Data_Observed
{
public:
    using Observer = Value_Conditional_Data_Observer<T, Condition>;
    using ObsList = std::vector<Observer *>;
    using ConList = std::vector<Condition>;

    virtual void attach(Observer * o, Condition c) {
        _observers.insert(_observers.begin(), o);
        _conditions.insert(_conditions.begin(), c);
    }

    virtual void detach(Observer * o, Condition c) {
        for (std::size_t i = _observers.size(); i-- > 0; ) {
            if (_observers[i] == o && _conditions[i] == c) {
                _observers.erase(_observers.begin() + i);
                _conditions.erase(_conditions.begin() + i);
            }
        }
    }

    virtual bool notify(Condition c, T d) {
        bool n = false;
        std::size_t size = _observers.size();

        for (std::size_t i = 0; i < size; ++i) {
            if (_conditions[i] == c) {
                if (_observers[i]) {
                    _observers[i]->update(this, c, d);
                    n = true;
                }
            }
        }
        return n;
    }

private:
    ObsList _observers;
    ConList _conditions;
};

#endif // VALUE_CONDITIONALLY_DATA_OBSERVED_H