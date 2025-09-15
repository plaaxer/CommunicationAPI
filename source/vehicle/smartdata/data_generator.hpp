#ifndef DATA_GENERATOR_HPP
#define DATA_GENERATOR_HPP

#include "vehicle/smartdata/smart_data.hpp"

#include <random>

// To avoid circular dependency
template<typename UnitTag> class Transducer;

/**
 * @details
 * Generates data for a Transducer. It is a simulation facility.
 * This source can be replaced with a Network source later.
 * For now, we will only use this to fetch random data.
 */
template<typename UnitTag>
class DataGenerator : Conditionally_Data_Observed<typename UnitTag::ValueType, int>
{
public:
    typedef Conditional_Data_Observer<typename UnitTag::ValueType, int> Observer;
    typedef Conditionally_Data_Observed<typename UnitTag::ValueType, int> Observed;
    typedef typename UnitTag::ValueType ValueType;

public:

    DataGenerator(Transducer<UnitTag>& t)
        : _transducer(t)
    {
        // Links the Transducer
        Observed::attach(static_cast<Conditional_Data_Observer<typename UnitTag::ValueType, int>*>(&_transducer), UnitTag::id);

        // Creates the thread to generate data
        _generator = std::thread(&DataGenerator::_gen_data_thread, this);

    }

    ~DataGenerator()
    {
        Observed::detach(_transducer, UnitTag::id);
        // Waits till the gen thread finish
        if(_generator.joinable()) {
            _generator.join();
        }
    }

private:

    /**
     * @brief Autonomous font of data. 
     */
    void _gen_data_thread()
    {
        while (true) {
            // Generates random int data. Only for test
            static thread_local std::mt19937 rng{ std::random_device{}() };
            static std::uniform_int_distribution<std::int64_t> dist(0, 1000);
            
            // cast before pass argument to transform rvalue to lvalue**
            ValueType value = static_cast<ValueType>(dist(rng));
            
            // Update the Transducer observing the generator
            Observed::notify(UnitTag::id, &value);

            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

private:
    std::thread _generator;  // Thread to periodically gen data
    Transducer<UnitTag>& _transducer;  // By reference to not make copy

};

#endif  // DATA_GENERATOR_HPP