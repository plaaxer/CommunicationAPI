#ifndef CONDITIONAL_DATA_OBSERVER_H
#define CONDITIONAL_DATA_OBSERVER_H

template <typename T, typename Condition>
class Conditionally_Data_Observed;

template <typename T, typename Condition = void>
class Conditional_Data_Observer {
    friend class Conditionally_Data_Observed<T, Condition>;

    public:
        typedef T ObsData;
        typedef Condition ObsCondition;

        virtual void update(Conditionally_Data_Observed<T, Condition> *obs, Condition c, T *d)
};

#endif // CONDITIONAL_DATA_OBSERVER_H