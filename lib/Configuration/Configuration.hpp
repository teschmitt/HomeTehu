#include <Arduino.h>
#include <FS.h>

class Configuration
{
private:
    const char *_filepath = "/config.json";
    fs::FS _fs;

    // preference data:
    bool debug;
    IPAddress MQTTHost;
    String sensorType = "DHT22";
    String stationName, wifiName, wifiPassword;
    uint8_t sensorGPIOPort;
    uint16_t MQTTPort = 1883;

    // all durations are specified in seconds
    uint16_t wifiTimeout = 60;
    uint16_t publishTimeout = 15;
    uint64_t sleepDuration = 300;

public:
    Configuration(fs::FS &fs);
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
    const uint16_t getWifiTimeout() const { return wifiTimeout; }
    const uint16_t getPublishTimeout() const { return publishTimeout; }
};
