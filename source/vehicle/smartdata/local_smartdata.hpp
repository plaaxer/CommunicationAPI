#ifndef LOCAL_SMART_DATA_HPP
#define LOCAL_SMART_DATA_HPP

#include "vehicle/smartdata/transducer.hpp"
#include "vehicle/smartdata/smart_data.hpp"

#include "api/network/communicator.hpp"

/**
 * @details
 * API end-point for manage data. Have an interface to get and set data
 * from a concrete Transducer (may be a Sensor or Actuator).
 */
template<typename Transducer>
class LocalSmartData : SmartData, Transducer::Observer
{
public:
    typedef typename Transducer::ValueType ValueType;    

public:
    LocalSmartData()
    {
        _transducer.attach(this, Transducer::Unit::id);
    }


    ~LocalSmartData()
    {
        // Free resources, detach, etc.
        _transducer.detach(this, Transducer::Unit::id);
    }


    /**
     * @brief SmartData interface to set data to the associated Transducer.
     */
    void operator=(ValueType* data)
    {
        // To verify if the associated Transducer is an Actuator

        _transducer.actuate();
    }

    /**
     * @details
     * Interface to get data from the associated Transducer calling sense().
     * It is an automatical casting when desired to manage ValueType values from a
     * LocalSmartData object. Like: 
     */
    operator ValueType()
    {
        // 1. Verify if the Transducer is of Sensor type LATER

        // 2. Fetch data from the associated Transducer
        ValueType* data = _transducer.sense();  // Why this way? To review with Hoffman

        return *data;
    }

    /**
     * TODO: Observer x Observed feed data to implement later
     */
    void update(Conditionally_Data_Observed<ValueType, int>* obs, int c, ValueType* d)
    {
        _updated_value = *d;
    }
    

private:
    Transducer _transducer;
    ValueType _updated_value;

};


#endif // LOCAL_SMART_DATA_HPP