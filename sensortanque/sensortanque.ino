
/*********************LIBRARY***************/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

/****************PIN Definitionz************/

#define TRIGGER 2
#define ECHO 4

/******************GLOBAL VARIABLES AND CONSTANTS ************/
int intDistance;
int intTime;
int intVolume=0;
int intFull=30;
int intEmpty=120;
int intCapacity=0;
int intTankRadiusCm=50;
int intLevelCm=0;
int intLevel=0;
float floatLitersPerCm=0.0;
float floatSpeedOfSoundCMPMS=0.0;

const char* ssid = "MILAGRITOS SALAS 5G";
const char* password ="alisito2023";

const int SerialSpeed=115200;
const int tempAvg=20;
const float SpeedOfSoundMPS=331 + (tempAvg*0.6);

IPAddress ip(192,168,1,38);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
WiFiServer server(80);
StaticJsonDocument<250> jsonDocument;
char bufferJson[250];





void setup() {
  // put your setup code here, to run once:

  floatSpeedOfSoundCMPMS=SpeedOfSoundsMPS*100/1000000;
  floatLitersPerCm=PI*(intTankRadiusCm*intTankRadiusCm)/1000;
  intCapacity=intEmpty-intFull;
  Serial.begin(SerialSpeed);
  Serial.print("Capacity: "); Serial.printl(intCapacity);
  Serial.print("Speed of sound Cm per uS: "); Serial.printl(floatSpeedOfSoundCMPMS);
  Serial.print("Liters per cm: "); Serial.println(floatLitersPerCm);
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT_PULLUP);
  Wifi.config(ip,gateway,subnet);
  Wifi.begin(ssid,password);
  Serial.print("Connecting to WiFi:");
  While(WiFi.status()!=WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }

  Serial.printl("\nConnected to WiFi");
  server.on("/tankStatus", getTankStatus);
  server.begin();

}

void loop() {


   /**CHECK DISTANCE***/

  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(15);
  digitalWrite(TRIGGER, LOW);
  intTime=pulseIn(ECHO, HIGH);
  intTime=intTime/2;
  intDistance=intTime*floatSpeedOfSoundCMPMS;
  Serial.print("Distance: "); Serial.println(intDistance);
  if(intDistance !=0){
    intLevelCm = intEmpty - intDistance;
  if(intLevelCm < 0)
    intLevelCm = 0;
  if(intLevelCm > intCapacity)
    intLevelCm = intCapacity;
  Serial.print("Nivel en CM: "); Serial.println(intLevelCm);
  Serial.print("CAPACITY"); Serial.println(intCapacity);
  intLevel=(float(intLevelCm)/float(intCapacity)) * 100;
  Serial.print("Nivel: "); Serial.print(intLevel); Serial.println("%");
  intVolume=intLevelCm*floatLitersPerCm;
  Serial.print("Volume: "); Serial.printl(intVolume);

  }
  else { // When intDistance is 0 is a sensor error or disconnected
  
   intLevel=-1;
   intVolume=-1;
  

  }
  /****CHECK WIFI CONNECTION AND CHECK WEBSERVER ***/

  if(WiFi.status() == WL_CONNECTED){
    server.handleClient();
  }
  else{
    Serial.printl("Connection lost");
    WiFi.disconnect();
    WiFi.reconnect();


  }
   delay(500);



}

   void getTankStatus(){

   Serial.println("Get tank status");
   Serial.printl("Get tank status");
   jsonDocument.clear();
   jsonDocument["level"] = intLevel;
   jsonDocument["volume"] = intVolume;
   serializeJson(jsonDocument,bufferJson);
   server.send(200,"application/json", bufferJson);



   }










