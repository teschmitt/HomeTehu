#ifndef __SENSOR_HPP
#define __SENSOR_HPP

#include "DHTesp.h"

class Sensor
{
public:
    Sensor() {};
    Sensor(uint8_t GPIOPort);

    const float getHumidity();
    const float getTemperature();

    const bool setGIOPort(uint8_t GPIOPort);

    ~Sensor();

private:
    DHTesp dht;

};

#endif /* __SENSOR_HPP */