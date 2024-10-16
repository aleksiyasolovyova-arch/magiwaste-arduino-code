#include <Arduino.h>
#include <Ultrasonic.h>

const int trigPin = 14;
const int echoPin = 13;

#define SOUND_SPEED 0.034

long duration;
float distanceCm;
float distanceInch;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

}

void loop() {
 digitalWrite(trigPin, LOW);
 delayMicroseconds(2);
 digitalWrite(trigPin, HIGH);
 delayMicroseconds(10);
 digitalWrite(trigPin, LOW);

 duration = pulseIn(echoPin, HIGH);

 distanceCm = duration * SOUND_SPEED/2;

 Serial.print("Distance:");
 Serial.println(distanceCm);
}