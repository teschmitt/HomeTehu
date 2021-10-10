#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <IPAddress.h>
#include <LittleFS.h>

#include "SensorBuffer.hpp"

SensorBuffer::SensorBuffer(fs::FS &fs) : _fs{fs}
{
    _bufferExists = _fs.exists(_bufferPath);
    if (_bufferExists)
    {
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
}

SensorBuffer::~SensorBuffer()
{
}

const bool SensorBuffer::bufferExists()
{
    return _bufferExists;
}

const DynamicJsonDocument SensorBuffer::getBuffer()
{
    DynamicJsonDocument res(4096);
    if (_bufferExists)
    {
        File bufferFile = _fs.open(_bufferPath, "r");
        if (!bufferFile)
        {
            Serial.println("ERROR: Couldn't open sensor buffer file. Got a big bad error!");
            // TODO: Handle errors
        }

        while (true)
        {
            StaticJsonDocument<256> doc;

            DeserializationError err = deserializeJson(doc, bufferFile);
            if (err)
                break;

            res.add(doc);
        }
        bufferFile.close();
    }
    return res;
}


/**
 * Augment a sensor buffer entry with current offset and append to persistent storage.
 *
 * @param doc JSON Object that is to be appended.
 * @param offset Offset that dates this sensor data
 * @return true if operation was successful
 */
const bool SensorBuffer::append(DynamicJsonDocument &doc, uint32_t offset)
{
    File bufferFile = _fs.open(_bufferPath, "a");
    if (!bufferFile)
    {
        Serial.println("ERROR: Couldn't open sensor buffer file. Got a big bad error!");
        // TODO: Handle errors
    }
    doc["offset"] = offset;
    serializeJson(doc, bufferFile);
    bufferFile.println();
    bufferFile.close();
    return true;
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
