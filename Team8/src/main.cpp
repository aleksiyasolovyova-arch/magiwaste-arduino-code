#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <DHT.h>
#include <DHT_U.h>

#define SOUND_SPEED 0.034
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 10       /* Time ESP32 will go to sleep (in seconds) */
#define DHTPIN 15
#define DHTTYPE DHT11
RTC_DATA_ATTR int bootCount = 0;

const int trigPin1 = 5;
const int echoPin1 = 18;

const int trigPin2 = 14;
const int echoPin2 = 27;

long duration1;
float distanceCm1;
long duration2;
float distanceCm2;

#define WIFI_SSID "KdG-iDev"
#define WIFI_PASSWORD "fquG9iCnQ4aa3Kca"

#define HOME_WIFI "Proximus-Home-E978"
#define HOME_PASSWD "wafa4667a559z"

// MQTT Setup
const char *mqtt_broker = "broker.emqx.io";
const char *topic = "emqx/esp32";
const char *mqtt_username = "admin";
const char *mqtt_password = "initial01";
const int mqtt_port = 8883;

String serverName = "http://10.134.178.158:8080/testdata";
String localHostName = "http://localhost:8080/h2-console/login.do";

DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wakeup caused by external signal using RTC_IO");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Wakeup caused by timer");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("Wakeup caused by touchpad");
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      Serial.println("Wakeup caused by ULP program");
      break;
    default:
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      break;
  }
}

void measureDistances() {
  digitalWrite(trigPin1, LOW);
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(20);
  digitalWrite(trigPin1, HIGH);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);
  digitalWrite(trigPin2, LOW);

  duration1 = pulseIn(echoPin1, HIGH);
  duration2 = pulseIn(echoPin2, HIGH);
  distanceCm1 = duration1 * SOUND_SPEED / 2;
  distanceCm2 = duration2 * SOUND_SPEED / 2;

  Serial.print("Distance 1 (cm): ");
  Serial.println(distanceCm1);
  Serial.print("Distance 2 (cm): ");
  Serial.println(distanceCm2);
}

void sendSensorDataToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Trying to reconnect...");
    WiFi.begin(HOME_WIFI, HOME_PASSWD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Reconnecting...");
    }
    Serial.println("Reconnected to WiFi.");
  }

  HTTPClient http;
  String serverName = "http://172.20.10.10/data";  
  http.begin(serverName);
  http.addHeader("Content-Type", "application/json");

  Serial.print("Sending data to server: ");
  Serial.println(serverName);

  StaticJsonDocument<200> doc;
  doc["sensorDistance1"] = distanceCm1;
  doc["sensorDistance2"] = distanceCm2;
  doc["Temperature"] = temperatureRead();
  String requestData;
  serializeJson(doc, requestData);

  Serial.print("Request Payload: ");
  Serial.println(requestData);

  int httpResponseCode = http.POST(requestData);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.print("Server response: ");
    Serial.println(response);
  } else {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(trigPin1, OUTPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(echoPin2, INPUT);

  bootCount++;
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_2, 1);

  WiFi.begin(HOME_WIFI, HOME_PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi.");

  dht.begin();
  Serial.println(F("DHT11 Unified Sensor Example"));
  
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  
  delayMS = 2000;
}

void loop() {
  delay(delayMS);
  measureDistances();

  if (WiFi.status() == WL_CONNECTED) {
    sendSensorDataToServer();
  }

  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
  }
  
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
  }

  delay(5000);  // 5 seconds before next loop
}
