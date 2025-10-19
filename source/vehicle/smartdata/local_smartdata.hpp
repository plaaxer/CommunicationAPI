#ifndef LOCAL_SMART_DATA_HPP
#define LOCAL_SMART_DATA_HPP

#include "vehicle/smartdata/transducer.hpp"
#include "vehicle/smartdata/smart_data.hpp"

/**
 * @details
 * End-point for managing data from a concrete Transducer.
 */
template<typename TransducerType>

class LocalSmartData : public TransducerType::Observer
{
public:

    using WrappedTransducer = TransducerType;
    using Value = typename TransducerType::Value;
    using Observed = typename TransducerType::Observed;

public:

    /**
     * @brief Attaches this instance to the transducer's TEDS type.
     */

    LocalSmartData() {

        _transducer.attach(this, TransducerType::UnitTagType);
    }

    ~LocalSmartData() {

        _transducer.detach(this, TransducerType::UnitTagType);
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
    TransducerType _transducer;
    Value _updated_value;
};

#endif // LOCAL_SMART_DATA_HPP