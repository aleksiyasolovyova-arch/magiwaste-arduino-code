#include <WiFi.h>
#include <Arduino.h> 
#include <WebSocketsServer.h>

#define LED 2
#define WIFI_SSID "Proximus-Home-E978"
#define WIFI_PASSWORD "wafa4667a559z"

WebSocketsServer webSocket = WebSocketsServer(80);

/**
 * WebSocketEvent will check based on what type its in, 
 * i have left the errors empty for now cuz idk how to do that yet
 * but the main connection all works
 */
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  switch (type){

    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected \n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connection from  \n", num);
        Serial.print(ip.toString());
      }
      break;

    case WStype_TEXT:
      {
        Serial.printf("[%u] Text: %s \n", num, payload);
        webSocket.sendTXT(num, payload);
      }
      break;

    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }

}


void setup(){
    Serial.begin(115200);
    Serial.println(WiFi.macAddress());
    pinMode(LED, OUTPUT);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected");
  Serial.print("My IP address:");
  Serial.println(WiFi.localIP());
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);



}
 
void loop(){
      digitalWrite(LED, WiFi.status() == WL_CONNECTED);
    webSocket.loop();
  
}
