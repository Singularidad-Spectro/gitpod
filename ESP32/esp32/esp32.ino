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
#include "WiFi.h"
#include <esp_now.h>

#include "ESP32_NOW.h"

//ESP-NOW
// Define a data structure
typedef struct struct_message {
    float a;
    float b;
    int c;
} struct_message;
 
// Create structured data object
struct_message myData;
 
// Callback function
void OnDataRecv(const esp_now_recv_info_t * esp_now_info, const uint8_t *incomingData, int len)
{
  // Get incoming data
  memcpy(&myData, incomingData, sizeof(myData));
  
  // Print to Serial Monitor
  Serial.print("Temp Sensor ");
  Serial.print(myData.c);
  Serial.print(": ");
  Serial.println(myData.a);
    
  Serial.print("Humidity Sensor ");
  Serial.print(myData.c);
  Serial.print(": ");
  Serial.println(myData.b); 
 
  Serial.println("");
}


// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
}


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

   // Set up Serial Monitor
  Serial.begin(115200);
 // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
 
  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

InitESPNow() ;
//esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t cb)
  //  InitESPNow();
  // Register callback function
  esp_now_register_recv_cb(OnDataRecv);

  




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
  Serial.println("la mac: "+ WiFi.macAddress()+ "  En núcleo -> " +  String(xPortGetCoreID()));
  delay(1000);
  cuenta++;
  Serial.println("desde loop -> " + String(cuenta));
}