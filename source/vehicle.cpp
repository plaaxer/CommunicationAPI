#include<cstdlib>
#include<ctime>
#include "sensor.cpp"
#include "communicator.h"

class Vehicle {
    private:
        Sensor velocimetro;
    public:
        Vehicle(Sensor v);
};