#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include "DHTesp.h"
#include <Ticker.h>
#include <ArduinoMqttClient.h>

// Constants

#define SOUND_SPEED 0.034
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 10       /* Time ESP32 will go to sleep (in seconds) */
#define DHTPIN 15
#define DHTTYPE DHT11

// KdG Wifi credentials
#define WIFI_SSID "KdG-iDev"
#define WIFI_PASSWORD "fquG9iCnQ4aa3Kca"

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "10.134.178.158";
int port = 1883;
const char topic[] = "test/topic";

const long interval = 1000;
unsigned long previousMillis = 0;

int count = 0;

// Pins US1
const int trigPin1 = 5;
const int echoPin1 = 18;

// Pins US2
const int trigPin2 = 19;
const int echoPin2 = 21;

// Pins tiltsensor
int tiltpin = 22;

// Pins IR sensor
const int irSensorPin = 4;
int sensorState;

// Globals
RTC_DATA_ATTR int bootCount = 0;
int tiltState = 0;
long duration1, duration2;
float sensorDistance1, sensorDistance2;
bool tilted = false;
bool wasteReceived = true;
String sensorData;

// temperute object
DHTesp dht;

/**
 * Under here you will find all the methods and paremeters used to calculate the temperature
 * most of which has been provided by the library and documentation of it but tuned to our specific needs.
 */
void tempTask(void *pvParameters);
bool getTemperature();
void triggerGetTemp();

/** Task handle for the light value read task */
TaskHandle_t tempTaskHandle = NULL;
/** Ticker for temperature reading */
Ticker tempTicker;
/** Comfort profile */
ComfortState cf;
/** Flag if task should run */
bool tasksEnabled = false;
/** Pin number for DHT11 data pin */
int dhtPin = 15;

bool initTemp()
{
  byte resultValue = 0;
  // Initialize temperature sensor
  dht.setup(dhtPin, DHTesp::DHT11);
  Serial.println("DHT initiated");

  // Start task to get temperature
  xTaskCreatePinnedToCore(
      tempTask,        /* Function to implement the task */
      "tempTask ",     /* Name of the task */
      4000,            /* Stack size in words */
      NULL,            /* Task input parameter */
      5,               /* Priority of the task */
      &tempTaskHandle, /* Task handle. */
      1);              /* Core where the task should run */

  if (tempTaskHandle == NULL)
  {
    Serial.println("Failed to start task for temperature update");
    return false;
  }
  else
  {
    // Start update of environment data every 10 seconds
    tempTicker.attach(10, triggerGetTemp);
  }
  return true;
}

/**
 * triggerGetTemp
 * Sets flag dhtUpdated to true for handling in loop()
 * called by Ticker getTempTimer
 */
void triggerGetTemp()
{
  if (tempTaskHandle != NULL)
  {
    xTaskResumeFromISR(tempTaskHandle);
  }
}

/**
 * Task to reads temperature from DHT11 sensor
 * @param pvParameters
 *    pointer to task parameters
 */
void tempTask(void *pvParameters)
{
  Serial.println("tempTask loop started");
  while (1) // tempTask loop
  {
    if (tasksEnabled)
    {
      // Get temperature values
      getTemperature();
    }
    // Got sleep again
    vTaskSuspend(NULL);
  }
}

/**
 * getTemperature
 * Reads temperature from DHT11 sensor
 * @return bool
 *    true if temperature could be aquired
 *    false if aquisition failed
 */
String getDHTSensorData(DHTesp &dht)
{
  // Reading temperature for humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  TempAndHumidity newValues = dht.getTempAndHumidity();
  // Check if any reads failed and exit early (to try again).
  if (dht.getStatus() != 0)
  {
    return "DHT11 error status: " + String(dht.getStatusString());
  }

  ComfortState cf;
  float heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
  float dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
  dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);

  return "T:" + String(newValues.temperature, 2) +
         " H:" + String(newValues.humidity, 2);
}

bool getTemperature()
{
  Serial.println(getDHTSensorData(dht)); // Or use the result as needed.
  return true;
}

/**
 * External wake up reason print statements, we only use 3 since we only want
 * to know if it has been waken up by the timer or the external active pin
 */
void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

void measureDistances()
{
  digitalWrite(trigPin1, LOW);
  delayMicroseconds(20);
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);
  duration1 = pulseIn(echoPin1, HIGH);

  digitalWrite(trigPin2, LOW);
  delayMicroseconds(20);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);
  duration2 = pulseIn(echoPin2, HIGH);

  sensorDistance1 = duration1 * SOUND_SPEED / 2;
  sensorDistance2 = duration2 * SOUND_SPEED / 2;

  Serial.print("Distance 1 (cm): ");
  Serial.println(sensorDistance1);
  Serial.print("Distance 2 (cm): ");
  Serial.println(sensorDistance2);
}

String debugJsonDoc(const JsonDocument &doc)
{
  String jsonOutput;
  serializeJson(doc, jsonOutput);
  return jsonOutput;
}

void connect()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected. Trying to reconnect...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      Serial.println("Reconnecting...");
    }
    Serial.println("Reconnected to WiFi.");
  }
}

void sendData(String protocol)
{
  connect();
  String comfortStatus;
  switch (cf)
  {
  case Comfort_OK:
    comfortStatus = "Comfort_OK";
    break;
  case Comfort_TooHot:
    comfortStatus = "Comfort_TooHot";
    break;
  case Comfort_TooCold:
    comfortStatus = "Comfort_TooCold";
    break;
  case Comfort_TooDry:
    comfortStatus = "Comfort_TooDry";
    break;
  case Comfort_TooHumid:
    comfortStatus = "Comfort_TooHumid";
    break;
  case Comfort_HotAndHumid:
    comfortStatus = "Comfort_HotAndHumid";
    break;
  case Comfort_HotAndDry:
    comfortStatus = "Comfort_HotAndDry";
    break;
  case Comfort_ColdAndHumid:
    comfortStatus = "Comfort_ColdAndHumid";
    break;
  case Comfort_ColdAndDry:
    comfortStatus = "Comfort_ColdAndDry";
    break;
  default:
    comfortStatus = "Unknown:";
    break;
  };

  StaticJsonDocument<200> doc;
  doc["deviceId"] = WiFi.macAddress();
  doc["sensorDistance1"] = sensorDistance1;
  doc["sensorDistance2"] = sensorDistance2;
  doc["tiltState"] = tilted;
  doc["temperature"] = dht.getTemperature();
  doc["humidity"] = dht.getHumidity();
  doc["comfortStatus"] = comfortStatus;
  doc["wasteReceived"] = wasteReceived;

  String requestData;
  serializeJson(doc, requestData);

  if (protocol == "HTTP")
  {
    HTTPClient http;
    String serverName = "http://10.134.178.158:8080/data";
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);

    Serial.println("Sending data to server through HTTP: " + serverName);
    Serial.println("Request Payload: " + requestData);
    Serial.println("Serialized JSON:");
    Serial.println(debugJsonDoc(doc));

    int httpResponseCode = http.POST(requestData);

    if (httpResponseCode > 0)
    {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Server response: " + response);
    }
    else
    {
      Serial.println("POST Request failed. Error: " + HTTPClient::errorToString(httpResponseCode));
    }
    http.end();
  }
  else if (protocol == "MQTT")
  {
    mqttClient.setId("esp1");
    mqttClient.setUsernamePassword("admin", "initial01");

    Serial.print("Attempting to connect to the MQTT broker: ");
    Serial.println(broker);

    int retryCount = 0;
    const int maxRetries = 5;
    while (!mqttClient.connect(broker, port) && retryCount < maxRetries)
    {
      Serial.print("Retrying MQTT connection... Attempt: ");
      Serial.println(retryCount + 1);
      delay(1000);
      retryCount++;
    }
    if (retryCount == maxRetries)
    {
      Serial.println("Failed to connect to MQTT broker after multiple attempts.");
      return;
    }

    Serial.println("Sending data to server through MQTT: ");
    Serial.println(debugJsonDoc(doc));

    mqttClient.poll();

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;

      Serial.print("Sending message to topic: ");
      Serial.println(requestData);
      Serial.println(count);

      mqttClient.beginMessage(topic);
      mqttClient.print(requestData);
      mqttClient.endMessage();

      Serial.println();

      count++;
    }
  }
  else
  {
    Serial.println("Unsupported protocol specified.");
  }
}

void setup()
{
  Serial.begin(115200);
  // Pin Configuration
  pinMode(trigPin1, OUTPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(echoPin2, INPUT);
  pinMode(tiltpin, INPUT);
  pinMode(irSensorPin, INPUT);
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected. Trying to reconnect...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      Serial.println("Reconnecting...");
    }
    Serial.println("Reconnected to WiFi.");
  }

  // Sensor Setup for temperature
  dht.setup(DHTPIN, DHTesp::DHT11);

  initTemp();
  tasksEnabled = true;

  print_wakeup_reason();

  bootCount++;
  Serial.println("Boot number: " + String(bootCount));

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 1);

  if (digitalRead(GPIO_NUM_4) == HIGH)
  {
    Serial.println("Tilt detected. Measuring and sending data...");
    measureDistances();
    sendData("HTTP");
    sendData("MQTT");
    String dhtData = getDHTSensorData(dht);
    Serial.println(dhtData);
  }
  else
  {
    Serial.println("No tilt detected. Going back to sleep...");
    measureDistances();
    sendData("HTTP");
    sendData("MQTT");
  }

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  Serial.println("Entering deep sleep...");
  delay(100);
}

void loop()
{
  measureDistances();
  sensorData = getDHTSensorData(dht);
  tiltState = digitalRead(tiltpin);
  sensorState = digitalRead(irSensorPin);

  if (tiltState == HIGH)
  {
    tilted = true;
    wasteReceived = false;

    sendData("MQTT");
  }
  else
  {
    tilted = false;
    if (sensorState == LOW)
    {
      Serial.println("Obstacle detected! Waste received.");

      wasteReceived = true;

      sendData("MQTT");
    }
  }

  delay(5000);
}