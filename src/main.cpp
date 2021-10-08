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

#include <PubSubClient.h>

// HomeTehu library imports
#include "Configuration.hpp"
#include "Sensor.hpp"

using namespace std;

// converting a struct tm to a JSON entry:
// https://arduinojson.org/news/2021/05/04/version-6-18-0/
bool convertToJson(const tm &src, JsonVariant dst)
{
  char buf[32];
  strftime(buf, sizeof(buf), "%FT%TZ", &src);
  return dst.set(buf);
}

Configuration config;
PubSubClient mqttClient;
Sensor sensor;
WiFiClient wifiClient;

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

void setup()
{
  time_t now = time(NULL);
  tm startUpTime = *gmtime(&now);

  Serial.begin(115200);
  Serial.println("Hello there, welcome to HomeTehu Station!");

  /* Read configuration from local filesystem ------------------------------------------------------------ */

  config.read();

  /* Set up sensor --------------------------------------------------------------------------------------- */
  sensor.setGIOPort(config.getSensorGPIOPort());

  /* Connect to wifi  ------------------------------------------------------------------------------------ */

  
  // TODO: Check if the next 3 lines are actually necessary:
  WiFi.mode(WIFI_OFF); //Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA); //This line hides the viewing of ESP as wifi hotspot
  Serial.println("Connecting to:");
  Serial.println(config.getWifiName());
  WiFi.begin(config.getWifiName(), config.getWifiPassword());

  while (WiFi.status() != WL_CONNECTED)
  { // Wait for the Wi-Fi to connect
    delay(250);
    // Serial.print(++i);
    // Serial.print(' ');
    // TODO: Set timeout and go back to sleep if not successful
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  /* Set up MQTT connection ------------------------------------------------------------------------------ */
  mqttClient.setClient(wifiClient);
  mqttClient.setServer(config.getMQTTHost(), config.getMQTTPort());
  // mqttClient.setBufferSize(512);
  mqttClient.setCallback(mqttSubCallback);
  String baseTopic = "/" + config.getStationName() + "/" + config.getSensorType() + "/";

  /* Send startup protocol ------------------------------------------------------------------------------- */

  DynamicJsonDocument payload(1024);
  String payloadSerialized;
  payload["startup_time"] = startUpTime;
  payload["status"] = "running";
  serializeJson(payload, payloadSerialized);

  /* 5. Read sensor data ------------------------------------------------------------------------------------ */
  const float humidity = sensor.getHumidity();
  const float temperature = sensor.getTemperature();

  Serial.println("Humidity: " + (String)humidity);
  Serial.println("Temperature : " + (String)temperature);

  /* 7. Plausibilität prüfen -------------------------------------------------------------------------------- */

  /* 8. Send data to server --------------------------------------------------------------------------------- */

  Serial.println("Connecting to MQTT broker at " + config.getMQTTHost().toString() + ":" + config.getMQTTPort() + " ... ");
  while (!mqttClient.connected())
  {
    if (mqttClient.connect(config.getStationName().c_str()))
    {
      Serial.println("Connected to MQTT");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  mqttClient.publish(
      (baseTopic + "temperature").c_str(),
      String(temperature).c_str());
  mqttClient.publish(
      (baseTopic + "humidity").c_str(),
      String(humidity).c_str());

  delay(2000);

  /* 9. Backups an Server schicken -------------------------------------------------------------------------- */

  /* 11. Sleep anfangen mit Intervall in Konfiguration ------------------------------------------------------ */
  WiFi.disconnect();
  Serial.print("Going to sleep for: ");
  Serial.println(config.getSleepDuration());
  ESP.deepSleep(config.getSleepDuration());
}

void loop() {}