#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <cstdlib>
#include <cstdint>

#include "api/observer/conditionally_data_observed.h"

struct EngineData { uint16_t rpm; uint8_t temperature_celsius; };
    
// Inherits from the unconditional observer (void condition)
class Engine: public Conditionally_Data_Observed<EngineData, void> 
{
public:
    void generate_new_data() {
        _data.rpm = 2000 + (rand() % 4000);
        _data.temperature_celsius = 85 + (rand() % 10);
        // Notify observers, passing a pointer to the data
        // this->notify(&_data);
    }
private:
    EngineData _data;
};

#endif // ENGINE_HPP