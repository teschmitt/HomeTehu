#include <ArduinoJson.h>
#include <FS.h>


class SensorBuffer
{
private:
    const char *_bufferPath = "/sensorbuffer.json";
    const char *_lastOffsetPath = "/lastoffset";
    uint32_t _lastOffset = 0;

    fs::FS _fs;
    
    bool _bufferExists = false;

public:
    SensorBuffer(fs::FS &fs);
    ~SensorBuffer();

    const bool append(String entry);
    const uint32_t getLastOffset();
    const uint32_t incrementOffset(uint32_t incr);
    const bool bufferExists();
    const bool read();
};
