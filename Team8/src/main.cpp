#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>


touch_pad_t touchPin;

#define SOUND_SPEED 0.034

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10      /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

const int trigPin1 = 5;
const int trigPin2 = 32;
const int echoPin1 = 18;
const int echoPin2 = 33;
const int tiltPin = 2;

long duration1;
float distanceCm1;
long duration2;
float distanceCm2;

#define WIFI_SSID "KdG-iDev"
#define WIFI_PASSWORD "fquG9iCnQ4aa3Kca"

// MQTT Setup

const char *mqtt_broker = "broker.emqx.io";
const char *topic = "emqx/esp32";
const char *mqtt_username = "admin";
const char *mqtt_password = "initial01";
const int mqtt_port = 8883;



/*
Method to print the reason by which ESP32
has been awaken from sleep
*/

String serverName = "http://10.134.178.158:8080/testdata";

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}


void binTilted(){
  digitalWrite(trigPin1, LOW);
  digitalWrite(trigPin2, LOW);

  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin1, HIGH);
  digitalWrite(trigPin2, HIGH);

  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);
  duration1 = pulseIn(echoPin1, HIGH);

  digitalWrite(trigPin2, LOW);
  duration2 = pulseIn(echoPin2, HIGH);

  // Calculate the distance
  distanceCm1 = duration1 * SOUND_SPEED/2;
  distanceCm2 = duration2 * SOUND_SPEED/2;

  // Convert to inches  
  // Prints the distance in the Serial Monitor
  Serial.print("Distance 1 (cm): ");
  Serial.println(distanceCm1);
  Serial.print("Distance 2 (cm): ");
  Serial.println(distanceCm2);

  delay(500);
  int sensorValue = digitalRead(tiltPin);
  if (sensorValue==HIGH)
  {
    Serial.println("BIN IS TILTED AAAA");
  } else{
    esp_deep_sleep_start();
  }
}


void setup() {
  Serial.begin(115200); // Starts the serial communication
  pinMode(trigPin1, OUTPUT); // Sets the trigPin as an Output
  pinMode(trigPin2, OUTPUT); // Sets the trigPin as an Output

  pinMode(echoPin1, INPUT); // Sets the echoPin as an Input
  pinMode(tiltPin, INPUT);
  pinMode(echoPin2, INPUT);

  /**
   * print every reboot
   */
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  /* First we configure the wake up source
  We set our ESP32 to wake up every 5 seconds
  */
  print_wakeup_reason();
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_2, 1);

}

void loop() {
  binTilted();  
}