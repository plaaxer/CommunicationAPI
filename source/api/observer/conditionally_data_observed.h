#ifndef CONDITIONALLY_DATA_OBSERVED_H
#define CONDITIONALLY_DATA_OBSERVED_H

#include<vector>

template <typename T, typename Condition>
class Conditional_Data_Observer;

template <typename T, typename Condition = void>
class Conditionally_Data_Observed
{
    friend class Conditional_Data_Observer<T, Condition>;

    public:
        typedef T ObsData;
        typedef Conditional_Data_Observer<T, Condition> Observer;
        typedef std::vector<Observer *> ObsList;
        typedef std::vector<Condition> ConList;

        virtual void attach(Observer * o, Condition c) {
            _observers.insert(_observers.begin(), o);
            _conditions.insert(_conditions.begin(), c);
        }

        virtual void detach(Observer * o, Condition c) {
            // remove all entries where both observer and condition match
            for (std::size_t i = _observers.size(); i-- > 0; ) {
                if (_observers[i] == o && _conditions[i] == c) {
                    _observers.erase(_observers.begin() + i);
                    _conditions.erase(_conditions.begin() + i);
                }
            }
        }

        virtual bool notify(Condition c, ObsData *d) {
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

#endif // CONDITIONALLY_DATA_OBSERVED_H