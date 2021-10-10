#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <IPAddress.h>
#include <LittleFS.h>

#include "SensorBuffer.hpp"

SensorBuffer::SensorBuffer(fs::FS &fs) : _fs{fs}
{
    _bufferExists = _fs.exists(_bufferPath);

    File offsetFile = _fs.open(_lastOffsetPath, "r");
    if (!offsetFile || offsetFile.isDirectory())
    {
        // Serial.println("ERROR: Couldn't open file with last offset. Got a big bad error!");
        // TODO: Handle errors
    }
    if (offsetFile.available())
        _lastOffset = offsetFile.parseInt();
    offsetFile.close();
}

SensorBuffer::~SensorBuffer()
{
}

const bool SensorBuffer::bufferExists()
{
    return _bufferExists;
}

const bool SensorBuffer::read()
{
    if (_bufferExists)
    {
        File logFile = _fs.open(_bufferPath, "r");
        if (!logFile)
        {
            Serial.println("ERROR: Couldn't open sensor buffer file. Got a big bad error!");
            // TODO: Handle errors
        }

        DynamicJsonDocument logDoc(8192);
        DeserializationError e = deserializeJson(logDoc, logFile);
        logFile.close();

        if (e)
        {
            Serial.println("ERROR: Deserialization. Got a big bad error!");
            // TODO Error handling
        }
    }
    else
    {
        return false;
    }
}

const bool SensorBuffer::append(String entry)
{
}

const uint32_t SensorBuffer::getLastOffset()
{
    return _lastOffset;
}

const uint32_t SensorBuffer::incrementOffset(uint32_t incr)
{
    uint32_t newOffset = _lastOffset + incr;
    File offsetFile = _fs.open(_lastOffsetPath, "w");
    if (!offsetFile || offsetFile.isDirectory())
    {
        // Serial.println("ERROR: Couldn't open file with last offset. Got a big bad error!");
        // TODO: Handle errors
    }
    if (offsetFile.available())
        _lastOffset = offsetFile.write(newOffset);
    offsetFile.close();
    return newOffset;
}