#ifndef DATA_GENERATOR_HPP
#define DATA_GENERATOR_HPP

#include "vehicle/smartdata/smart_data.hpp"
#include "api/network/definitions/teds.hpp"

#include "api/observer/value_conditional_data_observer.hpp"
#include "api/observer/value_conditionally_data_observed.hpp"

#include <random>
#include <type_traits>
#include <cstdint>
#include <thread>
#include <chrono>

// To avoid circular dependency
template<TEDS::Type UnitTag> class Transducer;

/**
 * @details
 * Generates data for a Transducer. It is a simulation facility.
 * This source can be replaced with a Network source later.
 * For now, we will only use this to fetch random data.
 */
template<TEDS::Type UnitTag>

class DataGenerator : public Value_Conditionally_Data_Observed<
                            std::conditional_t<((UnitTag & TEDS::FORMAT_MASK) == TEDS::FORMAT_FLOAT), float, int32_t>, 
                            TEDS::Type>
{
public:

    /**
     * Changes:
     * 1. Deriving Value from the Tag by applying a mask, rather than Value being a member of the Tag.
     * 2. Now passing TEDS::Type as the type in the second argument. 
     *    Used to be an int, because each semantic type could be represented with an int. 
     *    Now, we will use our Tag with the TYPE_MASK applied to it.
     *    Also switched from 'typedef typename' to 'using'.
     */

    // uses conditional to check if the Tag with the FORMAT_MASK applied is an int (32 bits) or a float, and uses the result in Value
    using Value = std::conditional_t<((UnitTag & TEDS::FORMAT_MASK) == TEDS::FORMAT_FLOAT), float, int32_t>;
    
    static constexpr TEDS::Type UnitTagType = (UnitTag & TEDS::TYPE_MASK);

    using Observer = Value_Conditional_Data_Observer<Value, TEDS::Type>;
    using Observed = Value_Conditionally_Data_Observed<Value, TEDS::Type>;

public:

    // TODO: 
    DataGenerator(Transducer<UnitTag>& t)
        : _transducer(t)
    {
        // Links the Transducer
        // BEFORE: Observed::attach(static_cast<Conditional_Data_Observer<typename UnitTag::ValueType, int>*>(&_transducer), UnitTag::id);
        // NOW:
        Observed::attach(&_transducer, UnitTagType);
        
        // Creates the thread to generate data
        _generator = std::thread(&DataGenerator::_gen_data_thread, this);

    }

    ~DataGenerator()
    {
        // Changes: Replaced 'UnitTag::id' with the UnitTag with the type mask applied, and passed transducer by reference
        Observed::detach(&_transducer, UnitTagType); 
        
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

            // TODO: Generate random int or random float based on Value being of type int or float

            // Generates random int data
            static thread_local std::mt19937 rng{ std::random_device{}() };
            static std::uniform_int_distribution<std::int64_t> dist(0, 1000);
            
            // casts before passing argument to transform rvalue to lvalue**
            Value value = static_cast<Value>(dist(rng));
            
            // Updates the Transducer observing the generator
            // Changes: UnitTag::id -> UnitTagType
            Observed::notify(UnitTagType, value); // POSSIBLE PROBLEM: should not pass value by reference

            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

private:
    std::thread _generator;  // Thread to periodically gen data
    Transducer<UnitTag>& _transducer;  // By reference to not make copy

};

#endif  // DATA_GENERATOR_HPP