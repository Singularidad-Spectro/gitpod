#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <ESP_Mail_Client.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ESP_Mail_Client.h>
#include <UrlEncode.h>
#include <WiFiManager.h>

TaskHandle_t Task1;


void loop2(void *parameter){
  for(;;){

    Serial.println("\t\t\t En nucleo -> " + String(xPortGetCoreID()));
    delay(500);

  }
  vTaskDelay(10);
}


void setup(){
  xTaskCreatePinnedToCore(
   loop2,
   "Task_1",
   1000,
   NULL,
   1,
   &Task1,
   0
   );
   Serial.begin(115200);

}


void loop(){
  Serial.println("En nucleo -> " + String(xPortGetCoreID()));
  delay(500);
}