#ifndef CONDITIONAL_DATA_OBSERVER_H
#define CONDITIONAL_DATA_OBSERVER_H

template <typename T, typename Condition>
class Conditionally_Data_Observed;

template <typename T, typename Condition = void>
class Conditional_Data_Observer {
    friend class Conditionally_Data_Observed<T, Condition>;

    public:

        using Observed_Data = T;
        using Observing_Condition = Condition;

        typedef T ObsData;
        typedef Condition ObsCondition;

        virtual void update(Conditionally_Data_Observed<T, Condition> *obs, Condition c, T *d) = 0;
};

template <typename T>
class Conditional_Data_Observer<T, void> {
  friend class Conditionally_Data_Observed<T, void>;

public:
  typedef T ObsData;

public:
  virtual void update(Conditionally_Data_Observed<T, void> *obs, T *d) = 0;
};


#endif // CONDITIONAL_DATA_OBSERVER_H