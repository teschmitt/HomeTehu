// Standard library imports
#include <time.h>

// external Arduino library imports
#include <Arduino.h>
#include <ArduinoJson.h>

#include <DHTesp.h>
#include <IPAddress.h>

// Todo find the correct libs for ESP32
#include <ESP8266WiFi.h>
// #include <ESP8266WiFiMulti.h>

#ifdef ESP32
#include <HTTPClient.h>
#else
#include <ESP8266HTTPClient.h>
#endif
#include <LittleFS.h>

#include <AsyncMqttClient.h>
#include <Ticker.h>
#include <WiFiUdp.h>

// HomeTehu library imports
#include "Configuration.hpp"
#include "Sensor.hpp"
#include "SensorBuffer.hpp"

using namespace std;

const bool convertToJson(const tm &src, JsonVariant dst);
void connectToWifi();

uint16_t _publish(const char *suffix, String msg);

/* Callback hell: */
void onWifiConnect(const WiFiEventStationModeGotIP &event);
void onWifiDisconnect(const WiFiEventStationModeDisconnected &event);
void mqttMsgCallback(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
void onMqttPublish(uint16_t packetId);
void connectToMqtt();

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
uint16_t unackedPubs;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

Configuration config(LittleFS);
SensorBuffer buf(LittleFS);

Sensor sensor;

/* If any of the connection steps fail (WiFi, MQTT, …), _offlineMode
 * will be toggled to true and sensor read data will be cached in a
 * ring buffer on flash memory until the next connection is possible.
 */
bool _offlineMode = false;

void setup()
{
  Serial.begin(115200);
  randomSeed(micros());

  Serial.println("Hello there, welcome to HomeTehu Station!");

  if (LittleFS.begin())
    Serial.println("LittleFS: Initialized");
  else
    Serial.println("LittleFS: Failed to initialize");

  config.read();

  sensor.setGIOPort(config.getSensorGPIOPort());

  String baseTopic = "/" + config.getStationName() + "/" + config.getSensorType() + "/";
  unackedPubs = 0;

  // mqttClient.onConnect(onMqttConnect);
  // mqttClient.onDisconnect(onMqttDisconnect);
  // mqttClient.onSubscribe(onMqttSubscribe);
  // mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(mqttMsgCallback);
  mqttClient.onPublish(onMqttPublish);

  mqttClient.setServer(config.getMQTTHost(), config.getMQTTPort());

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  connectToWifi();

  const float humidity = sensor.getHumidity();
  const float temperature = sensor.getTemperature();

  /* We have big plans here: Connecting to wifi and getting the sensor data should happen
   * async, then we need a barrier to wait for all connections to be established and the
   * sensor data to be available and first then publish it or buffer it if there are
   * connectivity issues.
   * There is a lot of busy waiting and not much error checking right now, but this should
   * be fixed in the future.
   * Helfpul for the necessary timeouts involved: https://stackoverflow.com/a/40551227
  **/

  /* Send data to server */
  if (_offlineMode)
  {
    // write to buffer
  }
  else
  {
    _publish("temperature", (String)temperature);
    _publish("humidity", (String)humidity);

    String log = String("Going to sleep for ") + (String)config.getSleepDuration() + (String) " seconds";
    _publish("log", log);

    if (buf.bufferExists())
    {
      // send all buffered data to server as well
    }
  }

  if (!_offlineMode)
  {
    bool _timedout = false;
    unsigned long _startTime = millis();
    while (unackedPubs > 0 && !_timedout)
    {
      delay(250);
      if ((millis() - _startTime) / 1000 >= config.getPublishTimeout())
        _timedout = true;
    }
    if (_timedout)
      Serial.println("One or more publishes remain unACKed");

    WiFi.disconnect();
  }

  Serial.print("Going to sleep for: ");
  Serial.println(config.getSleepDuration() + " seconds");
  ESP.deepSleep(config.getSleepDuration() * 1e6);
}

void loop() {}

void connectToWifi()
{
  bool _timedout = false;
  unsigned long _startTime = millis();
  WiFi.mode(WIFI_STA); // only station mode
  WiFi.begin(config.getWifiName(), config.getWifiPassword());

  while ((WiFi.status() != WL_CONNECTED) && !_timedout)
  { // Wait for the Wi-Fi to connect
    delay(250);
    if ((millis() - _startTime) / 1000 >= config.getWifiTimeout())
      _timedout = true;
  }
  if (_timedout)
    Serial.println("WARNING: Wifi connection failed, running in offline mode.");
  else
  {
    Serial.print("WiFi connected - IP address: ");
    Serial.println(WiFi.localIP());
  }
}

// converting a struct tm to a JSON entry:
// https://arduinojson.org/news/2021/05/04/version-6-18-0/
const bool convertToJson(const tm &src, JsonVariant dst)
{
  char buf[32];
  strftime(buf, sizeof(buf), "%FT%TZ", &src);
  return dst.set(buf);
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  Serial.println("Connected to Wi-Fi.");
  _offlineMode = false;
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  Serial.println("Disconnected from Wi-Fi.");
  _offlineMode = true;
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  // wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void mqttMsgCallback(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t length, size_t index, size_t total)
{
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(length);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
}

void onMqttPublish(uint16_t packetId)
{
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  unackedPubs--;
}

uint16_t _publish(const char *suffix, String msg)
{
  unackedPubs++;
  Serial.println("suffix: " + (String)suffix + " -- msg: " + msg + " -- unacked: " + unackedPubs);
  return mqttClient.publish(
      ("/" + config.getStationName() + "/" + config.getSensorType() + "/" + suffix).c_str(), 1, true,
      msg.c_str());
}