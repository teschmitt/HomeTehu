#include <Arduino.h>
#include <ArduinoJson.h>
#include <IPAddress.h>
#include <LittleFS.h>

#include "Configuration.hpp"

Configuration::Configuration(/* args */)
{
    randomSeed(micros());
    // TODO: perform begin in while loop and timeout after a while
    if (LittleFS.begin())
    {
        Serial.println("Dateisystem: initialisiert");
    }
    else
    {
        Serial.println("Dateisystem: Fehler beim initialisieren");
    }
}

Configuration::~Configuration()
{
}

const bool Configuration::read()
{
    File configFile = LittleFS.open(filepath, "r");

    if (!configFile)
    {
        Serial.println("ERROR: Couldn't open config file. Got a big bad error!");
        // TODO: Handle errors
    }

    DynamicJsonDocument configJSON(1024);
    DeserializationError e = deserializeJson(configJSON, configFile);
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
    sensorGPIOPort = configJSON["sensor_gpioport"];
    sensorType = configJSON["sensor_type"].as<String>();
    stationName = configJSON["station_name"].as<String>();
    wifiName = configJSON["wifi_name"].as<String>();
    wifiPassword = configJSON["wifi_password"].as<String>();
    sleepDuration = configJSON["sleep_duration"].as<uint32_t>() * 1e6;

    configFile.close();
    return true;
};

const bool Configuration::write()
{
    return false;
};