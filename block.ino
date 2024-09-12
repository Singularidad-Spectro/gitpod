// Using ESP8266
// using library DHT sensor Library by Adafruit Version 1.4.3
// This program for send temp, humidity to mqtt broker
// Receive message from mqtt broker to turn on device
// Reference GPIO  https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

/*********************LIBRARY***************/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
// #include <WiFi.h>
// #include <WebServer.h>
#include <ESP_Mail_Client.h>
// #include <HTTPClient.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP_Mail_Client.h>

#include <UrlEncode.h>

#include <WhatabotAPIClient.h>
#include <WiFiManager.h>
// Library for MQTT:
#include <PubSubClient.h>
/****************what bot **********/

#define WHATABOT_API_KEY ""
#define WHATABOT_CHAT_ID ""
#define WHATABOT_PLATFORM "whatsapp"
WiFiManager wifiManager;
WhatabotAPIClient whatabotClient(WHATABOT_API_KEY, WHATABOT_CHAT_ID, WHATABOT_PLATFORM);
#define AP_SSID " "
#define AP_PASS ""
/****************PIN Definitionz************/

#define TRIGGER 2
#define ECHO 4
#define LEVEL_SENSOR 34

/****************Mail************/
const char *user_base64 = "joffre.hermosilla@gmail.com";
const char *user_password_base64 = "";
const char *from_email = "MAIL From: <joffre.hermosilla@gmail.com>";
const char *to_email = "RCPT TO: <alucardaywalker@hotmail.com>";
uint32_t TIEMPO_DeepSleep = 90e6;
int contMail = 0;

/****************EMAIL************/
byte sendEmail(int x);
byte eRcv(WiFiClientSecure client);

/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials */
#define AUTHOR_EMAIL "joffre.hermosilla@gmail.com"
#define AUTHOR_PASSWORD ""

/* Recipient's email*/
#define RECIPIENT_EMAIL "alucardaywalker@hotmail.com"

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;

// Setup for DHT======================================
#include <DHT.h>
#define DHTPIN 2 // GPIO2 atau D4
// Uncomment the type of sensor in use:
// #define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE DHT11 // DHT 22 (AM2302)
// #define DHTTYPE    DHT21     // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;

// declare topic for publish message
const char *topic_pub = "ESP_Pub";
// declare topic for subscribe message
const char *topic_sub = "ESP_Sub";

// Update these with values suitable for your network.

const char *mqtt_server = "103.227.130.104";

// for output
int lamp1 = 16; // lamp for mqtt connected D0
int lamp2 = 5;  // lamp for start indicator D1
int lamp3 = 4;  // lamp for stop indicator D2

WiFiClient espClient;
PubSubClient client(espClient);
// char msg[50];

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

// WHATSAPP //
WiFiClient wifiClient;
HTTPClient http;
// COLOCAMOS EL TOKEN QUE NOS ENTREGA META
String token = "Bearer ";

// COLOCAMOS LA URL A DONDE SE ENVIAN LOS MENSAJES DE WHATSAPP
String servidor = "https://graph.facebook.com/v20.0//messages";
// CREAMOS UNA JSON DONDE SE COLOCA EL NUMERO DE TELEFONO Y EL MENSAJE
String payload = "{ \"messaging_product\": \"whatsapp\", \"to\": \"\", \"type\": \"template\", \"template\": { \"name\": \"hello_world\", \"language\": { \"code\": \"en_US\" } } }";
// PIN DEL SENSOR DE MOVIMIENTO
const int pinSensorMov = 15;
// ESTADO DEL SENSOR
int estadoActual = LOW;

// whatbot //
String phoneNumber = "";
String apiKey = "---";

/******************GLOBAL VARIABLES AND CONSTANTS ************/
int intPortValue = 0;
float floatLevelVolts = 0.0;
float floatLevelCm = 0.0;
float floatCapacityCm = 0.0;
int intLevelPercent = 0;

const float floatCmPerVolt = 200 / 2.4;
const int intTankRadiusCm = 55;

int intDistance;
int intTime;
int intVolume = 0;
int intFull = 30;
int intEmpty = 120;
int intCapacity = 0;
// int intTankRadiusCm=50;
int intLevelCm = 0;
int intLevel = 0;
float floatLitersPerCm = 0.0;
float floatSpeedOfSoundCMPMS = 0.0;

const char *ssid = " ";
const char *password = "";

const int SerialSpeed = 115200;
const int tempAvg = 20;
const float SpeedOfSoundMPS = 331 + (tempAvg * 0.6);
// conando en linux: nmcli dev show

IPAddress ip(192, 168, 1, 184);
IPAddress gateway(192, 168, 1, 1);
// conando en linux: ifconfig
IPAddress subnet(255, 255, 255, 0);

// IPAddress primaryDNS(8, 8, 8, 8);   //option
// IPAddress secondaryDNS(8, 8, 4, 4); //optional
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

// https://github.com/binaryupdates/NodeMCU-Webserver-Station-Mode/blob/main/ESP8266_Webserver_Station_Mode.ino#L10C3-L10C22

bool LEDstatus = LOW;

void sendMessage(String message)
{

  // Data to send with HTTP POST
  String url = "http://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);
  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Send HTTP POST request
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200)
  {
    Serial.print("Message sent successfully");
  }
  else
  {
    Serial.println("Error sending the message");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }

  // Free resources
  http.end();
}

void setup()
{
  //  wifiManager.autoConnect(AP_SSID, AP_PASS);
  wifiManager.autoConnect(ssid, password);
  // put your setup code here, to run once:
  float SpeedOfSoundsMPS;
  float floatSpeedOfSoundCMPMS = SpeedOfSoundsMPS * 100 / 1000000;
  float floatLitersPerCm = PI * (intTankRadiusCm * intTankRadiusCm) / 1000;
  int intCapacity = intEmpty - intFull;
  Serial.begin(SerialSpeed);
  Serial.setTimeout(timeoutTime);
  Serial.print("Capacity: ");
  Serial.print(intCapacity);
  Serial.print("Speed of sound Cm per uS: ");
  Serial.print(floatSpeedOfSoundCMPMS);
  Serial.print("Liters per cm: ");
  Serial.print(floatLitersPerCm);
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT_PULLUP);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi:");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.print("\nJoffre  CODER PATH WEB SERVER ESP8266 INICIANDO....");
  server.on("/", handle_OnConnect);
  server.on("/tankStatus", getTankStatus);
  server.on("/ledon", handle_ledon);
  server.on("/ledoff", handle_ledoff);
  server.onNotFound(handle_NotFound);

  server.begin();

  // nuevo

  // Initialize the output variables as outputs
  pinMode(output5, OUTPUT);
  pinMode(output4, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output5, LOW);
  digitalWrite(output4, LOW);

  // Configures static IP address
  if (!WiFi.config(ip, gateway, subnet))
  {
    Serial.println("STA Failed to configure");
  }

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  // Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  // DEEP SLEEP

  Serial.println("I'm awake, but I'm going into deep sleep mode for 5 minutes");
  // ESP.deepSleep(1500e6);

  /* get the wakeup reason
  ESP_SleepWakeupCause wakeupReason = ESP.getWakeupReason();
  switch (wakeupReason)
  {
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.println("Wakeup was not caused by deep sleep");
    break;
  } */

  // MAIL CON SMTP //
  // gmail_configuration();

  // Send Message to WhatsAPP by whatbot
  sendMessage("Hello Joffre, Soy el ESP8266 Reportando informe de IOT");

  // whatbot //

  whatabotClient.begin();
  whatabotClient.onMessageReceived(onMessageReceived);
  whatabotClient.onServerResponseReceived(onServerResponseReceived);

  // GMAIL SMPT ///

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /*  Set the network reconnection option */
  MailClient.networkReconnect(true);

  /** Enable the debug via Serial port
   * 0 for no debugging
   * 1 for basic level debugging
   *
   * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
   */
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the Session_Config for user defined session credentials */
  Session_Config config;

  /* Set the session config */
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";

  /*
  Set the NTP config time
  For times east of the Prime Meridian use 0-12
  For times west of the Prime Meridian add 12 to the offset.
  Ex. American/Denver GMT would be -6. 6 + 12 = 18
  See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
  */
  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = F("ESP8266 - CODER PATH");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("ESP8266 Test Email enviado desde ESP8266");
  message.addRecipient(F("Joffre"), RECIPIENT_EMAIL);

  /*Send HTML message*/
  /*String htmlMsg = "<div style=\"color:#2f4468;\"><h1>Hello World!</h1><p>- Sent from ESP board</p></div>";
  message.html.content = htmlMsg.c_str();
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;*/

  // Send raw text message
  String textMsg = "Hello World! - Mensaje enviado desde el microcontrolador esp8266 - lo lograste!!!";
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* Connect to the server */
  if (!smtp.connect(&config))
  {
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn())
  {
    Serial.println("\nNot yet logged in.");
  }
  else
  {
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());

  /// MQTT //
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  // subscribe topic
  client.subscribe(topic_sub);
  // setup pin output
  pinMode(lamp1, OUTPUT);
  pinMode(lamp2, OUTPUT);
  pinMode(lamp3, OUTPUT);
  // Reset lamp, turn off all Relay
  digitalWrite(lamp1, HIGH);
  digitalWrite(lamp2, HIGH);
  digitalWrite(lamp3, HIGH);
}

void loop()
{
  // what bot
  whatabotClient.loop();

  // tank status

  /*RANGE 0.6V - 3.0V : 12 Bits 0 -4095 : 0 - 2 M*/
  intPortValue = analogRead(LEVEL_SENSOR);
  Serial.println("----------------");
  Serial.print("intPortValue: ");
  Serial.println(intPortValue);
  floatLevelVolts = ((intPortValue * 3.3) / 4095) - 0.6;
  if (floatLevelVolts < 0.0)
    floatLevelVolts = 0;
  Serial.print("floatLevelVolts: ");
  Serial.println(floatLevelVolts);
  floatLevelCm = floatLevelVolts * floatCmPerVolt;
  Serial.print("floatLevelCm: ");
  Serial.println(floatLevelCm);

  Serial.print("Nivel en cm: ");
  Serial.println(floatLevelCm);

  intLevelPercent = (floatLevelCm / floatCapacityCm) * 100;
  Serial.print("Nivel: ");
  Serial.print(intLevelPercent);
  Serial.println("%");

  intVolume = floatLevelCm * floatLitersPerCm;
  Serial.print("Volume: ");
  Serial.println(intVolume);

  /**CHECK DISTANCE***/

  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(15);
  digitalWrite(TRIGGER, LOW);
  intTime = pulseIn(ECHO, HIGH);
  intTime = intTime / 2;
  intDistance = intTime * floatSpeedOfSoundCMPMS;
  Serial.print("/n Distance: ");
  Serial.print(intDistance);
  if (floatLevelVolts != 0)
  {
    /*     intLevelCm = intEmpty - intDistance;
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
   */
  }
  else
  { // When intDistance is 0 is a sensor error or disconnected

    intLevel = -1;
    intVolume = -1;
  }

  /****CHECK WIFI CONNECTION AND CHECK WEBSERVER ***/

  if (WiFi.status() == WL_CONNECTED)
  {

    server.handleClient();
    if (LEDstatus)
    {
      digitalWrite(output4, HIGH);
    }
    else
    {
      digitalWrite(output4, LOW);
    }
  }
  else
  {
    Serial.print("Connection lost");
    WiFi.disconnect();
    WiFi.reconnect();
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    // INICIAMOS EL OBJETO HTTP QUE POSTERIORMENTE ENVIARA EL MENSAJE
    HTTPClient httpx;
    // COLOCAMOS LA URL DEL SERVIDOR A DONDE SE ENVIARA EL MENSAJE
    httpx.begin(wifiClient, servidor.c_str());
    // COLOCAMOS LA CABECERA DONDE INDICAMOS QUE SERA TIPO JSON
    httpx.addHeader("Content-Type", "application/json");
    // AGREGAMOS EL TOKEN EN LA CABECERA DE LOS DATOS A ENVIAR
    httpx.addHeader("Authorization", token);
    // ENVIAMOS LOS DATOS VIA POST
    int httpPostCode = httpx.POST(payload);
    // SI SE LOGRARON ENVIAR LOS DATOS
    if (httpPostCode > 0)
    {
      // RECIBIMOS LA RESPUESTA QUE NOS ENTREGA META
      int httpResponseCode = httpx.GET();
      // SI HAY RESPUESTA LA MOSTRAMOS
      if (httpResponseCode > 0)
      {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = httpx.getString();
        Serial.println(payload);
      }
      else
      {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
    }
    httpx.end();
  }
  else
  {
    Serial.println("WiFi Desconectado");
  }

  delay(500);

  WiFiClient wificlient = server.client(); // Listen for incoming clients

  if (wificlient)
  {                                // If a new client connects,
    Serial.println("New Client."); // print a message out in the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (wificlient.connected() && currentTime - previousTime <= timeoutTime)
    { // loop while the client's connected
      currentTime = millis();
      if (wificlient.available())
      {                             // if there's bytes to read from the client,
        char c = wificlient.read(); // read a byte, then
        Serial.write(c);            // print it out the serial monitor
        header += c;
        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            wificlient.println("HTTP/1.1 200 OK");
            wificlient.println("Content-type:text/html");
            wificlient.println("Connection: close");
            wificlient.println();

            // turns the GPIOs on and off
            if (header.indexOf("GET /5/on") >= 0)
            {
              Serial.println("GPIO 5 on");
              output5State = "on";
              digitalWrite(output5, HIGH);
            }
            else if (header.indexOf("GET /5/off") >= 0)
            {
              Serial.println("GPIO 5 off");
              output5State = "off";
              digitalWrite(output5, LOW);
            }
            else if (header.indexOf("GET /4/on") >= 0)
            {
              Serial.println("GPIO 4 on");
              output4State = "on";
              digitalWrite(output4, HIGH);
            }
            else if (header.indexOf("GET /4/off") >= 0)
            {
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
            if (output5State == "off")
            {
              wificlient.println("<p><a href=\"/5/on\"><button class=\"button\">ON</button></a></p>");
            }
            else
            {
              wificlient.println("<p><a href=\"/5/off\"><button class=\"button button2\">OFF</button></a></p>");
            }

            // Display current state, and ON/OFF buttons for GPIO 4
            wificlient.println("<p>GPIO 4 - State " + output4State + "</p>");
            // If the output4State is off, it displays the ON button
            if (output4State == "off")
            {
              wificlient.println("<p><a href=\"/4/on\"><button class=\"button\">ON</button></a></p>");
            }
            else
            {
              wificlient.println("<p><a href=\"/4/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            wificlient.println("</body></html>");

            // The HTTP response ends with another blank line
            wificlient.println();
            // Break out of the while loop
            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
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

  if (sendEmail("Mensaje enviado con NODEMCU"))
  {
    Serial.println(F("Email sent"));
  }
  else
  {
    Serial.println(F("Email failed"));
  }

  contMail++;
  Serial.println(" En espera! ");
  // ESP.deepSleep(TIEMPO_DeepSleep, WAKE_NO_RFCAL);

  Serial.print("Email número: ");
  Serial.println(contMail);

  // MQTT //
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // read DHT sensor, temp and humidity-------------------------------
  t = dht.readTemperature();
  h = dht.readHumidity();
  if ((isnan(t)) || (isnan(h)))
  {
    Serial.println("Failed to read from DHT sensor!");
  }
}

void getTankStatus()
{

  Serial.print("Get tank status");
  Serial.print("Get tank status");
  jsonDocument.clear();
  jsonDocument["level"] = intLevel;
  jsonDocument["volume"] = intVolume;
  jsonDocument["WaterColumn"] = floatLevelCm;
  serializeJson(jsonDocument, bufferJson);
  server.send(200, "application/json", bufferJson);
}

void handle_OnConnect()
{
  LEDstatus = LOW;
  Serial.println("LED: OFF");
  server.send(200, "text/html", updateWebpage(LEDstatus));
}

void handle_ledon()
{
  LEDstatus = HIGH;
  Serial.println("LED: ON");
  server.send(200, "text/html", updateWebpage(LEDstatus));
}

void handle_ledoff()
{
  LEDstatus = LOW;
  Serial.println("LED: OFF");
  server.send(200, "text/html", updateWebpage(LEDstatus));
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

String updateWebpage(uint8_t LEDstatus)
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>CODER PATH ESP8266 PLATFORM</title>\n";
  ptr += "<link rel='shortcut icon' href='https://avatars.githubusercontent.com/u/145310760?s=400&u=a8d4e2b367b3d851668f549621cec1c9aec5193b&v=4' />\n";
  ptr += "<style>html {font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #3498db;}\n";
  ptr += ".button-on:active {background-color: #3498db;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>ESP8266 Web Server</h1>\n";
  ptr += "<h3>CODER PATH JOFFRE HERMOSILLA SALAS [DEVELOPER] </h3><img src='https://raw.githubusercontent.com/Hackathon-ChatGPT-NTTDATA/eurekaserver/master/src/main/resources/fotocreador/spring-logo-eureka.png' alt='CODER PATH' /> \n";
  ptr += "<h1> </h1>\n";
  ptr += "<a href='http://192.168.1.184/tankStatus'>TANQUE DE AGUA</a>\n";
  ptr += "<h1> </h1>\n";
  ptr += "<a href='/tankStatus'>Dos botones</a>\n";
  if (LEDstatus)
  {
    ptr += "<p>BLUE LED: ON</p><a class=\"button button-off\" href=\"/ledoff\">OFF</a>\n";
  }
  else
  {
    ptr += "<p>BLUE LED: OFF</p><a class=\"button button-on\" href=\"/ledon\">ON</a>\n";
  }

  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

byte sendEmail(String x)
{
  WiFiClient client = server.client();
  if (client.connect("mail.smtp2go.com", 587) == 1)
  {
    Serial.println(F("connected"));
  }
  else
  {
    Serial.println(F("connection failed"));
    return 0;
  }

  if (!eRcv(client))
    return 0;
  Serial.println(F("--- Sending EHLO"));
  client.println("EHLO 1.2.3.4");
  if (!eRcv(client))
    return 0;
  Serial.println(F("--- Sending login"));
  client.println("AUTH LOGIN");
  if (!eRcv(client))
    return 0;
  Serial.println(F("--- Sending User base64"));
  client.println(user_base64);
  if (!eRcv(client))
    return 0;
  Serial.println(F("--- Sending Password base64"));
  client.println(user_password_base64);
  if (!eRcv(client))
    return 0;
  Serial.println(F("--- Sending From"));
  client.println(from_email);
  if (!eRcv(client))
    return 0;
  Serial.println(F("--- Sending To"));
  client.println(to_email);
  if (!eRcv(client))
    return 0;
  Serial.println(F("--- Sending DATA"));
  client.println(F("DATA"));
  if (!eRcv(client))
    return 0;
  client.println(F("Subject: E-mail prueba NODEMCU\r\n"));
  client.println(x);
  client.println(F("."));

  if (!eRcv(client))
    return 0;
  Serial.println(F("--- Sending QUIT"));
  client.println(F("QUIT"));
  if (!eRcv(client))
    return 0;
  client.stop();
  return 1;
}

byte eRcv(WiFiClient client)
{
  byte respCode;
  byte thisByte;
  int loopCount = 0;

  while (!client.available())
  {
    delay(1);
    loopCount++;
    if (loopCount > 10000)
    {
      client.stop();
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }

  respCode = client.peek();
  while (client.available())
  {
    thisByte = client.read();
    Serial.write(thisByte);
  }
  if (respCode >= '4')
    return 0;
  return 1;
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success())
  {
    // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
    // In ESP8266 and ESP32, you can use Serial.printf directly.

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}

void gmail_configuration()
{

  /*  Set the network reconnection option */
  MailClient.networkReconnect(true);

  /** Enable the debug via Serial port
   * 0 for no debugging
   * 1 for basic level debugging
   *
   * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
   */
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the Session_Config for user defined session credentials */
  Session_Config config;

  /* Set the session config */
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";

  /*
  Set the NTP config time
  For times east of the Prime Meridian use 0-12
  For times west of the Prime Meridian add 12 to the offset.
  Ex. American/Denver GMT would be -6. 6 + 12 = 18
  See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
  */
  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = F("ESP");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("ESP Test Email");
  message.addRecipient(F("Sara"), RECIPIENT_EMAIL);

  /*Send HTML message*/
  /*String htmlMsg = "<div style=\"color:#2f4468;\"><h1>Hello World!</h1><p>- Sent from ESP board</p></div>";
  message.html.content = htmlMsg.c_str();
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;*/

  // Send raw text message
  String textMsg = "Hello World! - Sent from ESP board";
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* Connect to the server */
  if (!smtp.connect(&config))
  {
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn())
  {
    Serial.println("\nNot yet logged in.");
  }
  else
  {
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
}

void onServerResponseReceived(String message)
{
  Serial.println(message);
}

void onMessageReceived(String message)
{
  message.toUpperCase();
  Serial.println(message);

  if (message.equals("START"))
  {
    whatabotClient.sendMessageWS("Starting");
    // Add your logic for starting here
  }
  else if (message.equals("STOP"))
  {
    whatabotClient.sendMessageWS("Stopping");
    // Add your logic for stopping here
  }
  else if (message.equals("PAUSE"))
  {
    whatabotClient.sendMessageWS("Pausing");
    // Add your logic for pausing here
  }
  else if (message.equals("RESUME"))
  {
    whatabotClient.sendMessageWS("Resumming");
    // Add your logic for resuming here
  }
  else
  {
    whatabotClient.sendMessageWS("Unknown command");
    // Handle unknown commands here (optional)
  }
}

void setup_wifi()
{
  delay(100);
  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
  // Receiving message as subscriber
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  String json_received;
  Serial.print("JSON Received:");
  for (int i = 0; i < length; i++)
  {
    json_received += ((char)payload[i]);
    // Serial.print((char)payload[i]);
  }
  Serial.println(json_received);
  // if receive ask status from node-red, send current status of lamps
  if (json_received == "Status")
  {
    check_stat();
  }
  else
  {
    // Parse json
    // StaticJsonBuffer<200> jsonBuffer;  //arudion json v5
    // JsonObject& root = jsonBuffer.parseObject(json_received);

    JsonDocument doc;
    // JsonObject& JSONencoder = JSONbuffer.createObject();
    JsonObject root = doc.to<JsonObject>();

    // get json parsed value
    // sample of json: {"device":"Lamp1","trigger":"on"}
    Serial.print("Command:");
    String device = root["device"];
    String trigger = root["trigger"];
    Serial.println("Turn " + trigger + " " + device);
    Serial.println("-----------------------");
    // Trigger device
    // Lamp1***************************
    if (device == "Lamp1")
    {
      if (trigger == "on")
      {
        digitalWrite(lamp1, LOW);
      }
      else
      {
        digitalWrite(lamp1, HIGH);
      }
    }
    // Lamp2***************************
    if (device == "Lamp2")
    {
      if (trigger == "on")
      {
        digitalWrite(lamp2, LOW);
      }
      else
      {
        digitalWrite(lamp2, HIGH);
      }
    }
    // Lamp3***************************
    if (device == "Lamp3")
    {
      if (trigger == "on")
      {
        digitalWrite(lamp3, LOW);
      }
      else
      {
        digitalWrite(lamp3, HIGH);
      }
    }
    // All***************************
    if (device == "All")
    {
      if (trigger == "on")
      {
        digitalWrite(lamp1, LOW);
        digitalWrite(lamp2, LOW);
        digitalWrite(lamp3, LOW);
      }
      else
      {
        digitalWrite(lamp1, HIGH);
        digitalWrite(lamp2, HIGH);
        digitalWrite(lamp3, HIGH);
      }
    }
    check_stat();
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    // if you MQTT broker has clientID,username and password
    // please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      // once connected to MQTT broker, subscribe command if any
      client.subscribe(topic_sub);
      check_stat();
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void check_stat()
{
  // check output status--------------------------------------------
  // This function will update lamp status to mqtt
  // StaticJsonBuffer<300> JSONbuffer; //arudion json v5
  JsonDocument doc;
  // JsonObject& JSONencoder = JSONbuffer.createObject();
  JsonObject JSONencoder = doc.to<JsonObject>();

  bool stat_lamp1 = digitalRead(lamp1);
  bool stat_lamp2 = digitalRead(lamp2);
  bool stat_lamp3 = digitalRead(lamp3);
  // lamp1==========================
  if (stat_lamp1 == false)
  {
    JSONencoder["lamp1"] = true;
  }
  else
  {
    JSONencoder["lamp1"] = false;
  }
  // lamp2==========================
  if (stat_lamp2 == false)
  {
    JSONencoder["lamp2"] = true;
  }
  else
  {
    JSONencoder["lamp2"] = false;
  }
  // lamp3==========================
  if (stat_lamp3 == false)
  {
    JSONencoder["lamp3"] = true;
  }
  else
  {
    JSONencoder["lamp3"] = false;
  }

  JSONencoder["device"] = "ESP8266";
  JSONencoder["temperature"] = t;
  JSONencoder["humidity"] = h;

  char JSONmessageBuffer[100];
  // JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));// arduion json v5
  // serializeJson(JSONmessageBuffer);

  Serial.println("Sending message to MQTT topic..");
  Serial.println(JSONmessageBuffer);
  // JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  // serializeJson(JSONmessageBuffer);
  if (client.publish(topic_pub, JSONmessageBuffer) == true)
  {
    Serial.println("Success sending message");
  }
  else
  {
    Serial.println("Error sending message");
  }
  Serial.println("-------------");
}
