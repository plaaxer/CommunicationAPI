#include<cstdlib>
#include<ctime>
#include "sensor.cpp"

class Vehicle {
    private:
        Sensor velocimetro;
    public:
        Vehicle(Sensor v);
};