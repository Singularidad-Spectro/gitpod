
/*********************LIBRARY***************/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

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

const char* ssid = "MILAGRITOS SALAS";
const char* password ="alisito2023";

const int SerialSpeed=115200;
const int tempAvg=20;
const float SpeedOfSoundMPS=331 + (tempAvg*0.6);
// conando en linux: nmcli dev show 

IPAddress ip(192,168,1,184);
IPAddress gateway(192,168,1,1);
// conando en linux: ifconfig
IPAddress subnet(255,255,255,0);

//IPAddress primaryDNS(8, 8, 8, 8);   //option
//IPAddress secondaryDNS(8, 8, 4, 4); //optional
ESP8266WebServer server(80);
WiFiServer serverx(80);
StaticJsonDocument<250> jsonDocument;
char bufferJson[250];



// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output5State = "off";
String output4State = "off";

// Assign output variables to GPIO pins
const int output5 = 5;
const int output4 = 4;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

//https://github.com/binaryupdates/NodeMCU-Webserver-Station-Mode/blob/main/ESP8266_Webserver_Station_Mode.ino#L10C3-L10C22

bool LEDstatus = LOW;



void setup() {

   
  // put your setup code here, to run once:
  float SpeedOfSoundsMPS;
  float floatSpeedOfSoundCMPMS=SpeedOfSoundsMPS*100/1000000;
  float floatLitersPerCm=PI*(intTankRadiusCm*intTankRadiusCm)/1000;
  int intCapacity=intEmpty-intFull;
  Serial.begin(SerialSpeed);
 // pinMode(output4, OUTPUT);
  Serial.print("Capacity: "); Serial.print(intCapacity);
  Serial.print("Speed of sound Cm per uS: "); Serial.print(floatSpeedOfSoundCMPMS);
  Serial.print("Liters per cm: "); Serial.print(floatLitersPerCm);
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT_PULLUP);
  WiFi.config(ip,gateway,subnet);
  WiFi.begin(ssid,password);
  Serial.print("Connecting to WiFi:");
       while (WiFi.status()!=WL_CONNECTED){
    Serial.print(".");
    delay(500);

  }


  Serial.print("\nJOffre ");
  server.on("/", handle_OnConnect);
  server.on("/ledon", handle_ledon);
  server.on("/ledoff", handle_ledoff);
  server.onNotFound(handle_NotFound);
  server.on("/tankStatus", getTankStatus);
  server.begin();


//nuevo

 //  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(output5, OUTPUT);
  pinMode(output4, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output5, LOW);
  digitalWrite(output4, LOW);
  
  // Configures static IP address
  if (!WiFi.config(ip, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
 // Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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
  Serial.print("Distance: "); Serial.print(intDistance);
  if(intDistance !=0){
    intLevelCm = intEmpty - intDistance;
  if(intLevelCm < 0)
    intLevelCm = 0;
  if(intLevelCm > intCapacity)
    intLevelCm = intCapacity;
  Serial.print("Nivel en CM: "); Serial.print(intLevelCm);
  Serial.print("CAPACITY"); Serial.print(intCapacity);
  intLevel=(float(intLevelCm)/float(intCapacity)) * 100;
  Serial.print("Nivel: "); Serial.print(intLevel); Serial.print("%");
  intVolume=intLevelCm*floatLitersPerCm;
  Serial.print("Volume: "); Serial.print(intVolume);

  }
  else { // When intDistance is 0 is a sensor error or disconnected
  
   intLevel=-1;
   intVolume=-1;
  

  }
  /****CHECK WIFI CONNECTION AND CHECK WEBSERVER ***/

  if(WiFi.status() == WL_CONNECTED){
    server.handleClient();
     if(LEDstatus)
  {
    digitalWrite(output4, HIGH);}
  else
  {
    digitalWrite(output4, LOW);}
  }
  else{
    Serial.print("Connection lost");
    WiFi.disconnect();
    WiFi.reconnect();


  }
   delay(500);





 WiFiClient wificlient = serverx.available();   // Listen for incoming clients

  if (wificlient) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (wificlient.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (wificlient.available()) {             // if there's bytes to read from the client,
        char c = wificlient.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            wificlient.println("HTTP/1.1 200 OK");
            wificlient.println("Content-type:text/html");
            wificlient.println("Connection: close");
            wificlient.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /5/on") >= 0) {
              Serial.println("GPIO 5 on");
              output5State = "on";
              digitalWrite(output5, HIGH);
            } else if (header.indexOf("GET /5/off") >= 0) {
              Serial.println("GPIO 5 off");
              output5State = "off";
              digitalWrite(output5, LOW);
            } else if (header.indexOf("GET /4/on") >= 0) {
              Serial.println("GPIO 4 on");
              output4State = "on";
              digitalWrite(output4, HIGH);
            } else if (header.indexOf("GET /4/off") >= 0) {
              Serial.println("GPIO 4 off");
              output4State = "off";
              digitalWrite(output4, LOW);
            }
            
            // Display the HTML web page
            wificlient.println("<!DOCTYPE html><html>");
            wificlient.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            wificlient.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            wificlient.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            wificlient.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            wificlient.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            wificlient.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            wificlient.println("<body><h1>ESP8266 Web Server</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 5  
            wificlient.println("<p>GPIO 5 - State " + output5State + "</p>");
            // If the output5State is off, it displays the ON button       
            if (output5State=="off") {
              wificlient.println("<p><a href=\"/5/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              wificlient.println("<p><a href=\"/5/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for GPIO 4  
            wificlient.println("<p>GPIO 4 - State " + output4State + "</p>");
            // If the output4State is off, it displays the ON button       
            if (output4State=="off") {
              wificlient.println("<p><a href=\"/4/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              wificlient.println("<p><a href=\"/4/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            wificlient.println("</body></html>");
            
            // The HTTP response ends with another blank line
            wificlient.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    wificlient.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }



}

   void getTankStatus(){

   Serial.print("Get tank status");
   Serial.print("Get tank status");
   jsonDocument.clear();
   jsonDocument["level"] = intLevel;
   jsonDocument["volume"] = intVolume;
   serializeJson(jsonDocument,bufferJson);
   //server.send(200,"application/json", bufferJson);



   }



void handle_OnConnect() {
  LEDstatus = LOW;
  Serial.println("LED: OFF");
  server.send(200, "text/html", updateWebpage(LEDstatus)); 
}

void handle_ledon() {
  LEDstatus = HIGH;
  Serial.println("LED: ON");
  server.send(200, "text/html", updateWebpage(LEDstatus)); 
}

void handle_ledoff() {
  LEDstatus = LOW;
  Serial.println("LED: OFF");
  server.send(200, "text/html", updateWebpage(LEDstatus)); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String updateWebpage(uint8_t LEDstatus){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>LED Control</title>\n";
  ptr +="<style>html {font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #3498db;}\n";
  ptr +=".button-on:active {background-color: #3498db;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>ESP8266 Web Server</h1>\n";
  ptr +="<h3>Using Station(STA) Mode</h3>\n";
  
   if(LEDstatus){
    ptr +="<p>BLUE LED: ON</p><a class=\"button button-off\" href=\"/ledoff\">OFF</a>\n";
   }else{
    ptr +="<p>BLUE LED: OFF</p><a class=\"button button-on\" href=\"/ledon\">ON</a>\n";
   }

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}






