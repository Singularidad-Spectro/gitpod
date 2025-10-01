#include "SPI.h"
   #include "Adafruit_GFX.h"
   #include "Adafruit_GC9A01A.h"
   #include <NTPClient.h>

   #include <WiFiUdp.h>



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

//ESP-NOW
#include <espnow.h>

#include <SoftwareSerial.h>
#define RXp2 16  // D0 en NodeMCU
#define TXp2 17  // D1 en NodeMCU
SoftwareSerial serialLeonardo(RXp2, TXp2); // RX, TX
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -18000, 60000); // UTC-5 para Perú




/****************what bot **********/

#define WHATABOT_API_KEY "37fee9bc-ffc4-4ac9-964a"
#define WHATABOT_CHAT_ID "51989168761"
#define WHATABOT_PLATFORM "whatsapp"
WiFiManager wifiManager;
WhatabotAPIClient whatabotClient(WHATABOT_API_KEY, WHATABOT_CHAT_ID, WHATABOT_PLATFORM);
// Estructura para manejar múltiples redes WiFi
struct WiFiRed {
  const char* ssid;
  const char* password;
};

// Lista de redes WiFi disponibles
WiFiRed redes[] = {
  {"CONSTIJOFF-5G", "@2022Joy"},
  {"CONSTIJOFF_plus", "@2022Joy"},
  {"CONSTIJOFF", "@2022Joy"},
    {"Joffre", "1983joffre"},
  {"MILASALAS2025", "alisito2025"}  // Esta red se intentará conectar y también se usará como AP si falla todo
};

// La última red del array se usará como AP en caso de fallo
const int numRedes = sizeof(redes) / sizeof(redes[0]);
#define AP_SSID (redes[numRedes-1].ssid)
#define AP_PASS (redes[numRedes-1].password)
String ssid_conectado = "";

/****************PIN Definitionz************/

#define TRIGGER 2
#define ECHO 4

/***tanque de agua */

/************ PINES ************/
// Pin analógico del NodeMCU
#define LEVEL_SENSOR_T A0   // Amarillo del sensor -> divisor resistivo -> A0

// Pines ultrasónicos (si usas el sensor extra tipo HC-SR04)
#define TRIGGER_T D6
#define ECHO_T    D7

/************ VARIABLES ************/
//int intPortValue = 0;
//float floatLevelVolts = 0.0;
//float floatLevelCm = 0.0;
//int intLevelPercent = 0;
//int intVolume = 0;

// Ajusta según tu divisor resistivo y tanque
//float floatCmPerVolt = 100.0;     // Ejemplo: calibrar cm por volt
//float floatCapacityCm = 200.0;    // Altura total en cm
//float floatLitersPerCm = 3.75;    // Para tanque de 750 L / 200 cm altura
#define LEVEL_SENSOR 34

const float Rshunt = 150.0;   // resistencia en ohm
const float I_min = 0.0;      // corriente mínima en mA (tanque vacío)
const float I_max = 0.72;     // corriente máxima en mA (tanque lleno)
const float TankCapacity = 750.0; // litros




// Variables para control de servos
unsigned long lastServoCheck = 0;
unsigned long lastInclinacionCheck = 0;
int currentHourAngle = 0;
int currentInclinacionAngle = 0;

// Función para controlar el servo horario según la hora
void updateServoHorario() {
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  int newAngle = 0;
  
  // Mapeo de horas a ángulos
  if (currentHour == 5) {
    newAngle = 60;
  } else if (currentHour == 12) {
    newAngle = 120;
  } else if (currentHour == 15) {  // 3 PM
    newAngle = 180;
  } else if (currentHour == 20) {  // 8 PM
    newAngle = 0;
  } else if (currentHour > 5 && currentHour < 12) {
    // Interpolación entre 5 AM (60°) y 12 PM (120°)
    newAngle = map(currentHour * 60 + currentMinute, 5 * 60, 12 * 60, 60, 120);
  } else if (currentHour > 12 && currentHour < 15) {
    // Interpolación entre 12 PM (120°) y 3 PM (180°)
    newAngle = map(currentHour * 60 + currentMinute, 12 * 60, 15 * 60, 120, 180);
  }
  
  // Solo enviar comando si el ángulo ha cambiado
  if (newAngle != currentHourAngle) {
    String command = "H:" + String(newAngle);
    serialLeonardo.println(command);
    currentHourAngle = newAngle;
    Serial.println("Enviando comando horario: " + command);
  }
}

// Función para controlar la inclinación cada 8 horas
void updateServoInclinacion() {
  int currentHour = timeClient.getHours();
  int newAngle;
  
  // Cambiar inclinación cada 8 horas (0h, 8h, 16h)
  if (currentHour >= 16) {
    newAngle = 150;  // Tarde
  } else if (currentHour >= 8) {
    newAngle = 90;   // Mediodía
  } else {
    newAngle = 30;   // Mañana
  }
  
  // Solo enviar comando si el ángulo ha cambiado
  if (newAngle != currentInclinacionAngle) {
    String command = "I:" + String(newAngle);
    serialLeonardo.println(command);
    currentInclinacionAngle = newAngle;
    Serial.println("Enviando comando inclinación: " + command);
  }
}

///// reloj

   const long utcOffsetInSeconds = -18000;                                                                        // 3600 = western europe winter time - 7200 = western europe summer time
   char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


   #define TFT_DC  D2
   #define TFT_CS  D8
   #define DEG2RAD 0.0174532925   
   
// some extra colors
   #define BLACK      0x0000
   #define BLUE       0x001F
   #define RED        0xF800
   #define GREEN      0x07E0
   #define CYAN       0x07FF
   #define MAGENTA    0xF81F
   #define YELLOW     0xFFE0
   #define WHITE      0xFFFF
   #define ORANGE     0xFBE0
   #define GREY       0x84B5
   #define BORDEAUX   0xA000



/****************Mail************/
const char *user_base64 = "joffre.hermosilla@gmail.com";
const char *user_password_base64 = "1983joffre";
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
#define AUTHOR_PASSWORD "jtmn khfe nldj gsgp"

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

const char *mqtt_server = "broker.mqtt-dashboard.com";
// const char *mqtt_server = "2001:41d0:1:925e::1";
//  for output
int lamp1 = 16; // lamp for mqtt connected D0
int lamp2 = 5;  // lamp for start indicator D1
int lamp3 = 4;  // lamp for stop indicator D2




//configuracion ESP-NOW
// REPLACE WITH RECEIVER MAC Address
//3C:84:27:28:FA:F8 


uint8_t broadcastAddress[] = {0x3C, 0x84, 0x27, 0x28, 0xFA, 0xF8};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  char a[32];
  int b;
  float c;
  String d;
  bool e;
}
 struct_message;

// Create a struct_message called myData
struct_message myData;

unsigned long lastTime = 0;  
unsigned long timerDelay = 2000;  // send readings timer

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}








WiFiClient espClient;
PubSubClient client(espClient);
// char msg[50];

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

// WHATSAPP //
WiFiClient wifiClient;
HTTPClient http;
// COLOCAMOS EL TOKEN QUE NOS ENTREGA META
String token = "Bearer EAAPIgr6P6AABO8pZBKOb1RQZClIcmMMi9Q0S2mb5sFmtrLbSVirYV3aslDwUBeSzWQZB5rafBXFKf1XWFUBZBNRmrgQ3HgBv247X5L8l9PAhcT3217ZBNFWGTTT94hDVLGSpzQcMCw8oIIeTw88euwHLAfwCHSR348j6O1dpQ8wqSXcGZB7SuQCJ3vSkaZCj2l0";

// COLOCAMOS LA URL A DONDE SE ENVIAN LOS MENSAJES DE WHATSAPP
String servidor = "https://graph.facebook.com/v20.0/432760356580137/messages";
// CREAMOS UNA JSON DONDE SE COLOCA EL NUMERO DE TELEFONO Y EL MENSAJE
String payload = "{ \"messaging_product\": \"whatsapp\", \"to\": \"51989168761\", \"type\": \"template\", \"template\": { \"name\": \"hello_world\", \"language\": { \"code\": \"en_US\" } } }";
// PIN DEL SENSOR DE MOVIMIENTO
const int pinSensorMov = 15;
// ESTADO DEL SENSOR
int estadoActual = LOW;

// whatbot //
String phoneNumber = "51989168761";
String apiKey = "37fee9bc-ffc4-4ac9-964a";

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



// Las configuraciones de red se manejan desde el array redes[]

void conectarWiFiDinamico() {
  WiFi.mode(WIFI_STA);
  
  // Intentar conectar a cada red en el array (incluyendo la última)
  for(int i = 0; i < numRedes; i++) {
    Serial.print("\nIntentando conectar a ");
    Serial.println(redes[i].ssid);
    
    WiFi.begin(redes[i].ssid, redes[i].password);
    int intentos = 0;
    
    while (WiFi.status() != WL_CONNECTED && intentos < 20) {
      delay(500);
      Serial.print(".");
      intentos++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConexión exitosa");
      Serial.print("IP local: ");
      Serial.println(WiFi.localIP());
      ssid_conectado = redes[i].ssid;
      return;
    }
    
    Serial.println("\nNo se pudo conectar. Intentando siguiente red...");
    WiFi.disconnect();
    delay(1000);
  }

  // Si no se pudo conectar a ninguna red, iniciar modo AP
  Serial.println("\nNo se pudo conectar a ninguna red, iniciando modo AP...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP iniciado. IP: ");
  Serial.println(WiFi.softAPIP());
  ssid_conectado = String(AP_SSID) + " (AP)";
  Serial.print("SSID en modo AP: ");
  Serial.println(ssid_conectado);
}

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

// MQTT

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


//CALL ME BOT //
String phoneNumbercallmebot = "51989168761";
String apiKeycallmebot = "6550669";

void sendMessageCallmebot(String message){

  // Data to send with HTTP POST
  String mensaje_final = message + "\nSSID conectado: " + ssid_conectado;
  String url = "http://api.callmebot.com/whatsapp.php?phone=" + phoneNumbercallmebot + "&apikey=" + apiKeycallmebot + "&text=" + urlEncode(mensaje_final);
            //    "https://api.callmebot.com/whatsapp.php?phone=51989168761&text=This+is+a+test&apikey=6550669"
  WiFiClient client;    
  HTTPClient http;
  http.begin(client, url);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Send HTTP POST request
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200){
    Serial.print("Message sent successfully");
  }
  else{
    Serial.println("Error sending the message");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }

  // Free resources
  http.end();
}



void sendMessage(String message)
{

  // Data to send with HTTP POST
  String mensaje_final = message + "\nSSID conectado: " + ssid_conectado;
  String url = "http://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(mensaje_final);
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
//// reloj

 Adafruit_GC9A01A tft(TFT_CS, TFT_DC);

   float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;                              // saved H, M, S x & y multipliers
   float sdeg = 0, mdeg= 0, hdeg = 0;
   uint16_t osx = 120, osy = 120, omx = 120, omy = 120, ohx = 120, ohy = 120;          // saved H, M, S x & y coords
   uint16_t x0=0, x1=0, yy0=0, yy1=0;
   uint32_t targetTime = 0;                                                            // for next 1 second timeout

   int hh = 0;                                                                         // hours variable
   int mm = 0;                                                                         // minutes variable
   int ss = 0;                                                                         // seconds variable

   // NTP ya está configurado anteriormente  

bool initial = 1;



void setup()
{
  Serial.begin(9600);
  serialLeonardo.begin(9600);  // Inicializar comunicación con Leonardo
  
  // Inicializar NTP
  timeClient.begin();
  timeClient.setTimeOffset(-18000);  // UTC-5 para Perú
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

// comunicacion con arduino leonardo

//Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);


  /// reloj
 tft.begin (); 
   tft.setRotation (2);
   tft.fillScreen (BLACK);  
   delay (200);
   tft.fillScreen (RED);
   delay (200);
   tft.fillScreen (GREEN);
   delay (200);
   tft.fillScreen (BLUE);
   delay (200);
   tft.fillScreen (BLACK);  
   delay (200);
   tft.fillScreen (GREY);

   createDial ();

   Serial.begin (9600);
   Serial.println ();
   Serial.println ();
  conectarWiFiDinamico();
  Serial.println ("-------------------------------"); 
  timeClient.begin();
  timeClient.update ();
  Serial.print ("internet server time: ");   
  Serial.println(timeClient.getFormattedTime());
  hh = timeClient.getHours ();
  mm = timeClient.getMinutes ();
  ss = timeClient.getSeconds ();
  // put your setup code here, to run once:
  float SpeedOfSoundsMPS;
  float floatSpeedOfSoundCMPMS = SpeedOfSoundsMPS * 100 / 1000000;
  float floatLitersPerCm = PI * (intTankRadiusCm * intTankRadiusCm) / 1000;
  int intCapacity = intEmpty - intFull;
  Serial.begin(SerialSpeed);

  // whatbot //

  whatabotClient.begin();
  whatabotClient.onMessageReceived(onMessageReceived);
  whatabotClient.onServerResponseReceived(onServerResponseReceived);




  //tanque de agua
 pinMode(TRIGGER_T, OUTPUT);
  pinMode(ECHO_T, INPUT);
  digitalWrite(TRIGGER_T, LOW);

  Serial.println("Sistema de nivel iniciado");
  int adcVal = analogRead(LEVEL_SENSOR_T);
  float Vadc = (adcVal * 3.3) / 4095.0;
  float I_mA = (Vadc / Rshunt) * 1000.0;

  float Level_percent = (I_mA - I_min) / (I_max - I_min) * 100.0;
  if (Level_percent < 0) Level_percent = 0;
  if (Level_percent > 100) Level_percent = 100;

  float Volume_L = (Level_percent / 100.0) * TankCapacity;

 






  Serial.setTimeout(timeoutTime);
  Serial.println("------------------");
  Serial.print("ADC: "); Serial.println(adcVal);
  Serial.print("Vadc: "); Serial.println(Vadc, 3);
  Serial.print("I_mA: "); Serial.println(I_mA, 3);
  Serial.print("Nivel: "); Serial.print(Level_percent, 1); Serial.println(" %");
  Serial.print("Volumen: "); Serial.print(Volume_L, 1); Serial.println(" L");
  Serial.print("Capacity: ");
  Serial.print(intCapacity);
  Serial.print("Speed of sound Cm per uS: ");
  Serial.print(floatSpeedOfSoundCMPMS);
  Serial.print("Liters per cm: ");
  Serial.print(floatLitersPerCm);


  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT_PULLUP);
  WiFi.config(ip, gateway, subnet);
  conectarWiFiDinamico();
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
  if (!WiFi.config(ip, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  conectarWiFiDinamico();
  Serial.println("");
  Serial.println("WiFi conectado o AP iniciado.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  // DEEP SLEEP


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
String textMsg = String("Hello World! - Mensaje enviado desde el microcontrolador esp8266 - lo lograste!!! ") +
  " ADC: " + String(adcVal) +
  " Vadc: " + String(Vadc) +
  " I_mA: " + String(I_mA) +
  " Nivel: " + String(Level_percent) + " % " +
  " Volumen: " + String(Volume_L) + " L";
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

  // MQTT
  pinMode(BUILTIN_LED, OUTPUT); // Initialize the BUILTIN_LED pin as an output
  //  client.setCallback(callback_mqtt(char *topic, byte *payload, unsigned int length));


  // CALL ME BOT
   sendMessageCallmebot("Hello from ESP8266! EXITOSO MENSAJE DESDE CODER PATH " + WiFi.macAddress() );


     Serial.println("I'm awake, but I'm going into deep sleep mode for 5 hours");
  // ESP.deepSleep(15e6); //15 000 segundos = 4.17 horas 
  //ESP.deepSleep(18e+9, WAKE_RF_DEFAULT); 5 horas
}

void loop()
{

// ESP-NOW 

if ((millis() - lastTime) > timerDelay) {
    // Set values to send
    strcpy(myData.a, "THIS IS A CHAR");
    myData.b = random(1,20);
    myData.c = 1.2;
    myData.d = "Hello";
    myData.e = false;

    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    lastTime = millis();
  }


  // comucnicacion con arduino leonardo
   Serial.println("Message Received: ");
  //  Serial.println(Serial2.readString())



  // reloj

 if (targetTime < millis())
      {
      targetTime += 1000;
      ss++;                                                                            // advance second
      if (ss==60)
         {
         ss=0;
         mm++;                                                                         // advance minute
         if(mm>59)
            {
            mm=0;
            hh++;                                                                      // advance hour
            
            // Actualizar servos cada hora
            updateServoHorario();
            // Verificar si es hora de actualizar inclinación (cada 8 horas)
            if (hh % 8 == 0) {
              updateServoInclinacion();
            }
            if (hh>23) 
               {
               hh=0;
               timeClient.update ();                                                   // update at midnight
               }
            }
         }

           // 🔹 Sincronizar con NTP al inicio de cada minuto
    if (ss == 0) {
      timeClient.update();
      hh = timeClient.getHours();
      mm = timeClient.getMinutes();
      ss = timeClient.getSeconds();
    }
          
      // pre-compute hand degrees, x & y coords for a fast screen update
      sdeg = ss*6;                                                                     // 0-59 -> 0-354
      mdeg = mm*6+sdeg*0.01666667;                                                     // 0-59 -> 0-360 - includes seconds
      hdeg = hh*30+mdeg*0.0833333;                                                     // 0-11 -> 0-360 - includes minutes and seconds
      hx = cos ((hdeg-90)*DEG2RAD);    
      hy = sin ((hdeg-90)*DEG2RAD);
      mx = cos ((mdeg-90)*DEG2RAD);    
      my = sin ((mdeg-90)*DEG2RAD);
      sx = cos ((sdeg-90)*DEG2RAD);    
      sy = sin ((sdeg-90)*DEG2RAD);

      if (ss==0 || initial) 
         {
         initial = 0;
         tft.drawLine (ohx, ohy, 120, 121, BLACK);                                     // erase hour and minute hand positions every minute
         ohx = hx*62+121;    
         ohy = hy*62+121;
         tft.drawLine (omx, omy, 120, 121, BLACK);
         omx = mx*84+120;    
         omy = my*84+121;
         }
 
      tft.drawLine (osx, osy, 120, 121, BLACK);                                      // redraw new hand positions, hour and minute hands not erased here to avoid flicker
      osx = sx*90+121;    
      osy = sy*90+121;
      tft.drawLine (osx, osy, 120, 121, RED);
      tft.drawLine (ohx, ohy, 120, 121, WHITE);
      tft.drawLine (omx, omy, 120, 121, WHITE);
      tft.drawLine (osx, osy, 120, 121, RED);
      tft.fillCircle(120, 121, 3, RED);
      }


  // what bot
  whatabotClient.loop();


  // tank status

  /*RANGE 0.6V - 3.0V : 12 Bits 0 -4095 : 0 - 2 M*/
  intPortValue = analogRead(LEVEL_SENSOR_T);
  Serial.println("----------------");
  Serial.print("intPortValue: ");
  Serial.println(intPortValue);
  floatLevelVolts = ((intPortValue * 3.3) / 4095) - 0.6;
  if (floatLevelVolts < 0.0)
    floatLevelVolts = 0;
  Serial.println();
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  

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

  digitalWrite(TRIGGER_T, HIGH);
  delayMicroseconds(15);
  digitalWrite(TRIGGER_T, LOW);
  intTime = pulseIn(ECHO_T, HIGH);
  intTime = intTime / 2;
  intDistance = intTime * floatSpeedOfSoundCMPMS;
  Serial.print("/n Distance: ");
  Serial.print(intDistance);
  if (floatLevelVolts != 0)
  {
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

  // MQTT 2//

  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    value = analogRead(A0)*0.32;
    snprintf (msg, MSG_BUFFER_SIZE, "Temperature is :%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("device/temp", msg);
  }


// ====================== MENSAJES PROGRAMADOS ======================

// variable estática para evitar reenvíos en el mismo minuto
static int lastSentHour = -1;
static int lastSentMinute = -1;

// cuando sea la hora exacta (en minutos == 0)
if (mm == 0 && (hh == 0 || hh == 9 || hh == 12 || hh == 16 || hh == 21)) {

  // verificamos que no se haya enviado en este mismo minuto
  if (lastSentHour != hh || lastSentMinute != mm) {

    String mensaje = "Alerta automática: son las " + String(hh) + ":00 horas.";

    // -------- CallMeBot (WhatsApp) --------
 // CALL ME BOT
   sendMessageCallmebot(mensaje + " " + WiFi.macAddress() );

 // --------enviar correo --------

enviarCorreo(mensaje + " " + WiFi.macAddress() );

    // marcar que ya se envió en esta hora:minuto
    lastSentHour = hh;
    lastSentMinute = mm;
  }
}


// ====================== ALERTAS TANQUE DE AGUA ======================

// variable estática para evitar reenvíos en el mismo nivel
static int lastTankLevel = -1;

  // función que retorna distancia en cm desde sensor
//int nivel = map(intLevelPercent, intEmpty, intFull, 0, 100);  // 0% = vacío, 100% = lleno
int nivel = map(intLevelPercent, intEmpty, intFull, 0, 100);  // 0% = vacío, 100% = lleno
// limitar nivel entre 0 y 100
nivel = constrain(nivel, 0, 100);

// revisar solo si cambió de tramo (25, 50, 75, 100)
if ((nivel >= 100 || nivel >= 75 || nivel >= 50 || nivel >= 25) && nivel != lastTankLevel) {

  String mensajetanque = "Tanque de agua al " + String(nivel) + "%"   +" Con un volumen: "+intVolume +" litros";

   // -------- CallMeBot (WhatsApp) --------
 // CALL ME BOT
   sendMessageCallmebot(mensajetanque + " " + WiFi.macAddress() );

 // --------enviar correo --------

enviarCorreo(mensajetanque + " " + WiFi.macAddress() );


  // actualizar nivel enviado
  lastTankLevel = nivel;
}



}



void enviarCorreo(String mensaje){

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
  String textMsg = mensaje + "\nSSID conectado: " + ssid_conectado;
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
    Serial.println(redes[0].ssid);
    conectarWiFiDinamico();
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

  void setup_wifi_mqtt()
  {

    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(redes[0].ssid);

    conectarWiFiDinamico();

    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  void callback_mqtt(char *topic, byte *payload, unsigned int length)
  {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
      Serial.print((char)payload[i]);
    }
    Serial.println();

    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '1')
    {
      digitalWrite(BUILTIN_LED, LOW); // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is active low on the ESP-01)
    }
    else
    {
      digitalWrite(BUILTIN_LED, HIGH); // Turn the LED off by making the voltage HIGH
    }
  }

  void reconnect_mqtt()
  {
    // Loop until we're reconnected
    while (!client.connected())
    {
      Serial.print("Attempting MQTT connection...");
      // Create a random client ID
      String clientId = "ESP8266Client-";
      clientId += String(random(0xffff), HEX);
      // Attempt to connect
      if (client.connect(clientId.c_str()))
      {
        Serial.println("connected");
        // Once connected, publish an announcement...
        client.publish("device/temp", "MQTT Server is Connected");
        // ... and resubscribe
        client.subscribe("device/led");
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



void createDial (){

   tft.setTextColor (WHITE, GREY);  
   tft.fillCircle(120, 120, 118, BORDEAUX);                                           // creates outer ring
   tft.fillCircle(120, 120, 110, BLACK);   

   for (int i = 0; i<360; i+= 30)                                                     // draw 12 line segments at the outer ring 
      {                                                   
      sx = cos((i-90)*DEG2RAD);
      sy = sin((i-90)*DEG2RAD);
      x0 = sx*114+120;
      yy0 = sy*114+120;
      x1 = sx*100+120;
      yy1 = sy*100+120;
      tft.drawLine(x0, yy0, x1, yy1, GREEN);
      }
                                                             
   for (int i = 0; i<360; i+= 6)                                                      // draw 60 dots - minute markers
      {
      sx = cos((i-90)*DEG2RAD);
      sy = sin((i-90)*DEG2RAD);
      x0 = sx*102+120;
      yy0 = sy*102+120;    
      tft.drawPixel(x0, yy0, WHITE);
    
      if(i==0  || i==180) tft.fillCircle (x0, yy0, 2, WHITE);                         // draw main quadrant dots
      if(i==90 || i==270) tft.fillCircle (x0, yy0, 2, WHITE);
     }
  
   tft.fillCircle(120, 121, 3, WHITE);                                               // pivot
   targetTime = millis() + 1000;   
}