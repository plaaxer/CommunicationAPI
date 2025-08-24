#ifndef SENSOR_HPP
#define SENSOR_HPP

class Sensor
{
private:
    int type = 0;
    float reading;
    int address;

public:
    Sensor(int a);
    float get_reading();
    void update();
    void start();
    pid_t pid;
};

#endif // SENSOR_HPP