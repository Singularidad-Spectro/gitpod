
/*********************LIBRARY***************/

#include <Wifi.h>
#include <WebServer.h>
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

const char* ssid = "MILAGRITOS SALAS 5G"
const char* password ="alisito2023"

const int SerialSpeed=115200;
const int tempAvg=20;
const float SpeedOfSoundMPS=331 + (tempAvg*0.6);

IPAddress ip(192.168.1.38);
IPAddress gateway(192.168.1.1);
IPAddress subnet(255.255.255.0);
WebServer server(80);
StaticJsonDocument<250> jsonDocument;
char bufferJson[250];





void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
