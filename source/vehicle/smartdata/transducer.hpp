#ifndef TRANSDUCER_HPP
#define TRANSDUCER_HPP

#include "api/observer/conditionally_data_observed.h"
#include "api/observer/conditional_data_observer.hpp"
#include "vehicle/smartdata/smart_data.hpp"
#include "vehicle/smartdata/data_generator.hpp"

/**
 * @details
 * The bridge between hardware (sensor or actuator) and the LocalSmartData API end-point
 * TODO: Build specific Transducers specialized for each sensor/actuator type
 * TODO: Implement the Observer x Observed. The Transducer and data source must be
 * independents threads to make it possible.
 */
template<typename UnitTag>  // To improve specification here
class Transducer : SmartData, 
                   Conditionally_Data_Observed<typename UnitTag::ValueType, int>,
                   public Conditional_Data_Observer<typename UnitTag::ValueType, int>  // public to do upcast*
{
public:
    typedef Conditional_Data_Observer<typename UnitTag::ValueType, int> Observer;
    typedef Conditionally_Data_Observed<typename UnitTag::ValueType, int> Observed;
    typedef typename UnitTag::ValueType ValueType;
    typedef UnitTag Unit;

public:
    Transducer ()
        : _data(0), _data_generator(new DataGenerator<UnitTag>(*(this)))
    {}

    void attach(Observer* obs, int c)
    {
        Observed::attach(obs, 0);
    }

    void detach(Observer* obs, int)
    {
        Observed::detach(obs, 0);
    }

    /**
     * @brief Only for sensors
     */
    ValueType* sense() { return _data; }

    /**
     * @brief Only for actuators
     * TODO: Implement actuate stack logic (LATER)
     */
    void actuate(ValueType data);


private:
    /**
     * @brief Feeded by a DataGenerator or by Network supply. DO NOT USE NOW
     */
    void update(Observed* obs, int c, ValueType* d)
    {
        _data = d;  // To review pointer questions

        // Notifies API end-point with updated data 
        Observed::notify(UnitTag::id, _data);
    }
    
    private:
    ValueType* _data;
    DataGenerator<UnitTag>* _data_generator;
};


#endif  // TRANSDUCER_HPP