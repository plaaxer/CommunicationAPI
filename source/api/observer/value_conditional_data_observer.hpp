#ifndef VALUE_CONDITIONAL_DATA_OBSERVER_H
#define VALUE_CONDITIONAL_DATA_OBSERVER_H

template <typename T, typename Condition>
class Value_Conditionally_Data_Observed;

/**
 * @brief An observer that receives data by VALUE (copy).
 * This is ideal for small, simple data types like int, float, or small structs.
 */
template <typename T, typename Condition = void>
class Value_Conditional_Data_Observer {
    friend class Value_Conditionally_Data_Observed<T, Condition>;

public:
    using Observed_Data = T;
    using Observing_Condition = Condition;

    virtual void update(Value_Conditionally_Data_Observed<T, Condition> *obs, Condition c, T d) = 0;
};

// Specialization for when there is no condition
template <typename T>
class Value_Conditional_Data_Observer<T, void> {
    friend class Value_Conditionally_Data_Observed<T, void>;

public:
    using Observed_Data = T;

    virtual void update(Value_Conditionally_Data_Observed<T, void> *obs, T d) = 0;
};

#endif // VALUE_CONDITIONAL_DATA_OBSERVER_H