#include <Arduino.h>

class Configuration
{
private:
    const char *filepath = "/config.json";

    // preference data:
    bool debug;
    IPAddress MQTTHost;
    String sensorType, stationName, wifiName, wifiPassword;
    uint8_t sensorGPIOPort;
    uint16_t MQTTPort;
    uint64_t sleepDuration;

public:
    Configuration(/* args */);
    ~Configuration();

    const bool read();
    const bool write();

    const bool getDebug() const { return debug; }
    const IPAddress getMQTTHost() const { return MQTTHost; }
    const uint16_t getMQTTPort() const { return MQTTPort; }
    const String getSensorType() const { return sensorType; }
    const String getStationName() const { return stationName; }
    const String getWifiName() const { return wifiName; }
    const String getWifiPassword() const { return wifiPassword; }
    const uint64_t getSleepDuration() const { return sleepDuration; }
    const unsigned int getSensorGPIOPort() const { return sensorGPIOPort; }
};
