#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <IPAddress.h>
#include <LittleFS.h>

#include "Configuration.hpp"

Configuration::Configuration(fs::FS &fs): _fs{fs}
{
}

Configuration::~Configuration()
{
}

const bool Configuration::read()
{
    File configFile = _fs.open(_filepath, "r");

    if (!configFile)
    {
        Serial.println("ERROR: Couldn't open config file. Got a big bad error!");
        // TODO: Handle errors
    }

    DynamicJsonDocument configJSON(1024);
    DeserializationError e = deserializeJson(configJSON, configFile);
    configFile.close();

    if (e)
    {
        Serial.println("ERROR: Deserialization. Got a big bad error!");
        // TODO Error handling
    }

    debug = configJSON["debug"];

    IPAddress ipaddr;
    ipaddr.fromString((const char *) configJSON["mqtt_host"]);
    MQTTHost = ipaddr;

    MQTTPort = configJSON["mqtt_port"];
    wifiTimeout = configJSON["wifi_timeout"].as<uint16_t>();
    publishTimeout = configJSON["publish_timeout"].as<uint16_t>();
    sensorGPIOPort = configJSON["sensor_gpioport"];
    sensorType = configJSON["sensor_type"].as<String>();
    stationName = configJSON["station_name"].as<String>();
    wifiName = configJSON["wifi_name"].as<String>();
    wifiPassword = configJSON["wifi_password"].as<String>();
    sleepDuration = configJSON["sleep_duration"].as<uint32_t>();

    return true;
};

const bool Configuration::write()
{
    return false;
};