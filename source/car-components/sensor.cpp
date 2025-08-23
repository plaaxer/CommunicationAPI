#include<cstdlib>
#include<ctime>
#include <unistd.h>

class Sensor {
    private: 
        int type = 0;
        float reading;
    public:
        float get_reading();
        void update();
        void start();
        pid_t pid;
};

void Sensor::start(){
    pid = fork();
    if (pid < 0) {
        return;
    } else if (pid == 0){
        while(true) {
            update();
        };
    };
};

float Sensor::get_reading() {
    return reading;
}

void Sensor::update() {
    float speed = 40 + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(60-40)));
    reading = speed;
}
