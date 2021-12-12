#include "Sensor.hpp"

#include "DHTesp.h"

Sensor::Sensor(uint8_t GPIOPort)
{
    setGIOPort(GPIOPort);
}

const float Sensor::getHumidity()
{
    const float hum = dht.getHumidity();
    if (isnan(hum)) {
        // get wicked!
    } else {
        return hum;
    }
    return hum;
}

const float Sensor::getTemperature()
{
    const float temp = dht.getTemperature();
    if (isnan(temp)) {
        // get wicked!
    } else {
        return temp;
    }
    return temp;
}

const bool Sensor::setGIOPort(uint8_t GPIOPort) {
    dht.setup(GPIOPort, DHTesp::DHT22);

    // Nach der Initialisierung warten, damit wir sampling period
    // nicht unterschreiten. Später mal Timestamp des letzten Samples
    // abrufen und vergleichen
    delay(dht.getMinimumSamplingPeriod());
    return true;
}

Sensor::~Sensor()
{
}
