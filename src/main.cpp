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

/* Callback hell: */
void onWifiConnect(const WiFiEventStationModeGotIP &event);
void onWifiDisconnect(const WiFiEventStationModeDisconnected &event);
void mqttMsgCallback(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
void onMqttPublish(uint16_t packetId);
void connectToMqtt();

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

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

  /* Mount cute little filesystem --------------------------------------------------------------------------- */
  // TODO: perform begin in while loop and timeout after a while
  if (LittleFS.begin())
  {
    Serial.println("LittleFS: Initialized");
  }
  else
  {
    Serial.println("LittleFS: Failed to initialize");
  }

  /* Read configuration from local filesystem --------------------------------------------------------------- */

  config.read();

  /* Set up sensor ------------------------------------------------------------------------------------------ */
  sensor.setGIOPort(config.getSensorGPIOPort());

  /* Set up MQTT connection --------------------------------------------------------------------------------- */
  String baseTopic = "/" + config.getStationName() + "/" + config.getSensorType() + "/";

  // mqttClient.onConnect(onMqttConnect);
  // mqttClient.onDisconnect(onMqttDisconnect);
  // mqttClient.onSubscribe(onMqttSubscribe);
  // mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(mqttMsgCallback);
  mqttClient.onPublish(onMqttPublish);

  mqttClient.setServer(config.getMQTTHost(), config.getMQTTPort());

  /* Connect to wifi  --------------------------------------------------------------------------------------- */

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  connectToWifi();


  /* Read sensor data --------------------------------------------------------------------------------------- */
  const float humidity = sensor.getHumidity();
  const float temperature = sensor.getTemperature();

  Serial.println("Humidity: " + (String)humidity);
  Serial.println("Temperature : " + (String)temperature);

  /* Send data to server ------------------------------------------------------------------------------------ */
  if (_offlineMode)
  {
    // write to buffer
  }
  else

  
  {
    mqttClient.publish(
        (baseTopic + "temperature").c_str(), 1, true,
        String(temperature).c_str());
    mqttClient.publish(
        (baseTopic + "humidity").c_str(), 1, true,
        String(humidity).c_str());
  

    if (buf.bufferExists())
    {
      // send all buffered data to server as well
    }

    delay(500); // without delay MQTT messages sometimes don't go through before shutdown
  }

  /* Backups an Server schicken -------------------------------------------------------------------------- */

  /* Sleep anfangen mit Intervall in Konfiguration ------------------------------------------------------ */
  if (!_offlineMode)
    WiFi.disconnect();

  Serial.print("Going to sleep for: ");
  Serial.println(config.getSleepDuration() + " seconds");
  ESP.deepSleep(config.getSleepDuration() * 1e6);
}

void loop() {}

void connectToWifi()
{
  bool _timedout = false;
  unsigned long _startTime = millis();
  // WiFi.mode(WIFI_OFF); //Prevents reconnection issue (taking too long to connect)
  // delay(1000);
  WiFi.mode(WIFI_STA); //This line hides the viewing of ESP as wifi hotspot
  Serial.println("Connecting to:");
  Serial.println(config.getWifiName());
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
  wifiReconnectTimer.once(2, connectToWifi);
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
}
