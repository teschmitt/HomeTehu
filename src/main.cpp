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

#include <PubSubClient.h>
#include <WiFiUdp.h>

// HomeTehu library imports
#include "Configuration.hpp"
#include "Sensor.hpp"

using namespace std;

const bool convertToJson(const tm &src, JsonVariant dst);
const bool connectToMQTT(PubSubClient &c);
const bool connectToWifi();
void mqttSubCallback(char *topic, uint8_t *payload, uint16_t length);

Configuration config;
PubSubClient mqttClient;
Sensor sensor;
WiFiClient wifiClient;

/* If any of the connection steps fail (WiFi, MQTT, …), _offlineMode
 * will be toggled to true and sensor read data will be cached in a
 * ring buffer on flash memory until the next connection is possible.
 */
bool _offlineMode = false;

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello there, welcome to HomeTehu Station!");

  /* Mount cute little filesystem --------------------------------------------------------------------------- */

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

  /* Start building run protocol ---------------------------------------------------------------------------- */

  DynamicJsonDocument runLogPayload(1024);
  runLogPayload["status"] = "running";

  /* Read configuration from local filesystem --------------------------------------------------------------- */

  config.read();

  /* Set up sensor ------------------------------------------------------------------------------------------ */
  sensor.setGIOPort(config.getSensorGPIOPort());

  /* Connect to wifi  --------------------------------------------------------------------------------------- */

  if (!connectToWifi())
  {
    _offlineMode = true;
    runLogPayload["wifi_mode"] = "offline";
  }
  else
  {
    runLogPayload["wifi_mode"] = "online";
  }

  /* Set up MQTT connection --------------------------------------------------------------------------------- */
  String baseTopic = "/" + config.getStationName() + "/" + config.getSensorType() + "/";
  if (!_offlineMode)
  {
    mqttClient.setClient(wifiClient);
    mqttClient.setServer(config.getMQTTHost(), config.getMQTTPort());
    mqttClient.setBufferSize(1024);
    mqttClient.setCallback(mqttSubCallback);
  }

  /* Read sensor data --------------------------------------------------------------------------------------- */
  const float humidity = sensor.getHumidity();
  const float temperature = sensor.getTemperature();

  Serial.println("Humidity: " + (String)humidity);
  Serial.println("Temperature : " + (String)temperature);
  runLogPayload["sensor_reading"]["temperature"] = temperature;
  runLogPayload["sensor_reading"]["humidity"] = humidity;

  /* Send data to server ------------------------------------------------------------------------------------ */
  if (_offlineMode)
  {
    // write to log
  }
  else
  {
    bool _mqttOffline = !connectToMQTT(mqttClient);
    if (_mqttOffline)
    {
      runLogPayload["mqtt_mode"] = "offline";
    }
    else
    {
      runLogPayload["mqtt_mode"] = "online";
      mqttClient.publish(
          (baseTopic + "temperature").c_str(),
          String(temperature).c_str());
      mqttClient.publish(
          (baseTopic + "humidity").c_str(),
          String(humidity).c_str());

      delay(500); // without delay MQTT messages sometimes don't go through before shutdown
    }
  }
  /* 9. Backups an Server schicken -------------------------------------------------------------------------- */

  String runLogPayloadSerialized;
  serializeJson(runLogPayload, runLogPayloadSerialized);

  /* 11. Sleep anfangen mit Intervall in Konfiguration ------------------------------------------------------ */
  if (!_offlineMode)
    WiFi.disconnect();

  Serial.print("Going to sleep for: ");
  Serial.println(config.getSleepDuration());
  ESP.deepSleep(config.getSleepDuration());
}

void loop() {}

const bool connectToMQTT(PubSubClient &c)
{
  Serial.println("Connecting to MQTT broker at " + config.getMQTTHost().toString() + ":" + config.getMQTTPort() + " ... ");
  while (!c.connected())
  {
    if (c.connect(config.getStationName().c_str()))
    {
      Serial.println("Connected to MQTT");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(c.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  return c.connected();
}

const bool connectToWifi()
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
    // Serial.print(++i);
    // Serial.print(' ');
    // TODO: Set timeout and go back to sleep if not successful
  }
  if (_timedout)
  {
    Serial.println("WARNING: Wifi connection failed, running in offline mode.");
  }
  else
  {
    Serial.print("WiFi connected - IP address: ");
    Serial.println(WiFi.localIP());
  }
  return (WiFi.status() == WL_CONNECTED);
}

// converting a struct tm to a JSON entry:
// https://arduinojson.org/news/2021/05/04/version-6-18-0/
const bool convertToJson(const tm &src, JsonVariant dst)
{
  char buf[32];
  strftime(buf, sizeof(buf), "%FT%TZ", &src);
  return dst.set(buf);
}

void mqttSubCallback(char *topic, uint8_t *payload, uint16_t length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (uint16_t i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}