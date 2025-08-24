#ifndef CONDITIONALLY_DATA_OBSERVED_H
#define CONDITIONALLY_DATA_OBSERVED_H
#include<list>

template <typename T, typename Condition>
class Conditional_Data_Observer;

template <typename T, typename Condition = void>
class Conditionally_Data_Observed
{
    friend class Conditional_Data_Observer<T, Condition>;

    public:
        typedef T ObsData;
        typedef Conditional_Data_Observer<T, Condition> Observer;
        typedef std::list<Observer *> ObsList;
        typedef std::list<Condition> ConList;

        virtual void attach(Observer * o, Condition c) {
            _observers.push_front(o);
            _conditions.push_front(c);
        }

        virtual void dettach(Observer * o, Condition c) {
            _observers.remove(o);
            _conditions.remove(c);
        }

        virtual bool notify(Condition c, ObsData *d) {
            bool n = false;
            std::size_t size = _observers.size();
            int range = static_cast<int>(size);
            for(i = 0; i < range; i++) {
                if (_conditions[i] == c) {
                    _observers[i]->update(this, c, d);
                    n = true;
                }
            }
        }

    private:
        ObsList _observers;
        ConList _conditions;

};

#endif // CONDITIONALLY_DATA_OBSERVED_H