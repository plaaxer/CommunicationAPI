#ifndef TRANSDUCER_HPP
#define TRANSDUCER_HPP

// Includes for the NEW pass-by-value observer pattern
#include "api/observer/value_conditionally_data_observed.hpp"
#include "api/observer/value_conditional_data_observer.hpp"

#include "vehicle/smartdata/data_generator.hpp"

/**
 * @details
 * The bridge between a data source (DataGenerator) and the end-point (LocalSmartData).
 */
template<TEDS::Type UnitTag>
class Transducer : public Value_Conditionally_Data_Observed< // we are observed by local smart data.
                        typename DataGenerator<UnitTag>::Value, 
                        TEDS::Type
                   >,
                   public Value_Conditional_Data_Observer< // we observe the data generator.
                        typename DataGenerator<UnitTag>::Value, 
                        TEDS::Type
                   >
{
public:

    // Value type refers to what type (float, int) the data is represented on, and not the teds type.
    using Value = typename DataGenerator<UnitTag>::Value;

    using ConditionType = TEDS::Type;

    using Observer = Value_Conditional_Data_Observer<Value, ConditionType>;
    using Observed = Value_Conditionally_Data_Observed<Value, ConditionType>;

    static constexpr TEDS::Type UnitTagType = DataGenerator<UnitTag>::UnitTagType;

public:

    Transducer()
        : _data(),
          _data_generator(*this)
    {}

    void attach(Observer* obs, ConditionType c) {
        Observed::attach(obs, c);
    }
    void detach(Observer* obs, ConditionType c) {
        Observed::detach(obs, c);
    }

    Value sense() const {
        return _data;
    }

    void actuate(Value data) {
    }

private:

    void update(Value_Conditionally_Data_Observed<Value, ConditionType>* obs, ConditionType c, Value d) override {
        _data = d;

        // notifying the next layer in the chain (LocalSmartData) with a fresh copy of the data.
        Observed::notify(UnitTagType, _data);
    }
    
private:

    Value _data;
    DataGenerator<UnitTag> _data_generator;
};

#endif  // TRANSDUCER_HPP