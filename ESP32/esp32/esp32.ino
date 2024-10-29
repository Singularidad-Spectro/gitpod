/*#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <ESP_Mail_Client.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <UrlEncode.h>
#include <WiFiManager.h>
*/

TaskHandle_t Task2, Task3;
int cuenta = 0;

void loop2(void *parameter){
  for(;;){
    Serial.println("\t\t\tEn núcleo -> " +  String(xPortGetCoreID()));
    delay(100);
    cuenta++;
    Serial.println("desde loop 2 -> " + String(cuenta));
  }
  vTaskDelay(10);
}

void loop3(void *parameter){
  for(;;){
    Serial.println("\t\t\t Loop 3 🤯🤯🤯");
    delay(2000);
    cuenta++;
    Serial.println("desde loop 3 -> " + String(cuenta));
  }
  vTaskDelay(10);
}

void setup() {

  xTaskCreatePinnedToCore(
    loop2,
    "Task_2",
    1000,
    NULL,
    1,
    &Task2,
    0);

  xTaskCreatePinnedToCore(
      loop3,
      "Task_3",
      1000,
      NULL,
      1,
      &Task3,
      0);

  Serial.begin(115200);
}

void loop() {
  Serial.println("En núcleo -> " +  String(xPortGetCoreID()));
  delay(1000);
  cuenta++;
  Serial.println("desde loop -> " + String(cuenta));
}