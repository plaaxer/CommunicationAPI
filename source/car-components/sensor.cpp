#include<cstdlib>
#include<ctime>

class Sensor {
    private: 
        float reading;
    public:
        float get_reading();
        void update();
};

float Sensor::get_reading() {
    return reading;
}

void Sensor::update() {
    float speed = 40 + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(60-40)));
    reading = speed;
}
