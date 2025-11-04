#ifndef LOCAL_SMART_DATA_HPP
#define LOCAL_SMART_DATA_HPP

#include "vm/vehicle/smartdata/transducer.hpp"
#include "vm/vehicle/smartdata/smart_data.hpp"

/**
 * @details
 * End-point for managing data from a concrete Transducer.
 */
template<typename Transducer>

class LocalSmartData : public Transducer::Observer
{
public:

    using WrappedTransducer = Transducer;
    using Value = typename Transducer::Value;
    using Observed = typename Transducer::Observed;

public:

    /**
     * @brief Attaches this instance to the transducer's TEDS type.
     */

    LocalSmartData() {

        _transducer.attach(this, Transducer::UnitTagType);
    }

    ~LocalSmartData() {

        _transducer.detach(this, Transducer::UnitTagType);
    }

    /**
     * @brief SmartData interface to set data to the associated Transducer.
     */
    void operator=(Value data) {

        _transducer.actuate(data);
    }

    /**
     * @details Interface to get data. Returns the last value received via update.
     */
    operator Value() {

        return _updated_value;
    }

    void update(Value_Conditionally_Data_Observed<Value, TEDS::Type>* obs, TEDS::Type c, Value d) override {
        _updated_value = d;
    }

private:
    Transducer _transducer;
    Value _updated_value;
};

#endif // LOCAL_SMART_DATA_HPP