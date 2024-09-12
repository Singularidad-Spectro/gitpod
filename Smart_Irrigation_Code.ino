#include "arduino_secrets.h"
/*
  Sketch generated by the Arduino IoT Cloud Thing 

  Arduino IoT Cloud Variables description

  The following variables are automatically generated and updated when changes are made to the Thing

  float current_Temp;
  float currrent_Humid;
  int current_Moisture;
  int trigger_Level;
  bool pump_Status;

  Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
  which are called when their values are changed from the Dashboard.
  These functions are generated with the Thing and added at the end of this sketch.
*/

// Adafruit GFX Library - Version: Latest
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>

// Adafruit SSD1306 - Version: Latest
#include <Adafruit_SSD1306.h>
#include <splash.h>

// DHT sensor library - Version: Latest
#include <DHT.h>
#include <DHT_U.h>

// Wire Library for I2C
#include <Wire.h>

// Set OLED size in pixels
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Set OLED parameters
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Define DHT22 Parameters
#define DHTPIN 8
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Define variables for Temperature and Humidity
float temp;
float hum;

// Sensor constants - replace with values from calibration sketch

// Constant for dry sensor
const int DryValue = 2650;

// Constant for wet sensor
const int WetValue = 1800;

// Variables for soil moisture
int soilMoistureValue;
int soilMoisturePercent;

// Analog input port
#define SENSOR_IN 0

// Relay Port
#define RELAY_OUT 3

// Pump Status Text
String pump_status_text = "OFF";

// Variable for pump trigger
int pump_trigger = 30;

// IoT Cloud Thing Properties file
#include "thingProperties.h"

void setup() {

  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500);

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you'll get.
     The default is 0 (only errors).
     Maximum is 4
  */
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  // Initialize I2C display using 3.3 volts
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();

  // Initialize DHT22
  dht.begin();

  // Set ADC to use 12 bits
  analogReadResolution(12);

  // Set Relay as Output
  pinMode(RELAY_OUT, OUTPUT);

  // Turn off relay
  digitalWrite(RELAY_OUT, LOW);

  // Set Pump Status to Off
  pump_Status = false;

}

void loop() {
  // Call Arduino IoT Cloud for updates
  ArduinoCloud.update();

  // Get temperature and Humidity
  temp = dht.readTemperature();
  hum = dht.readHumidity();

  // Pass temperature and humidity values to cloud variables
  current_Temp = temp;
  currrent_Humid = hum;

  // Get soil mositure value
  soilMoistureValue = analogRead(SENSOR_IN);

  // Determine soil moisture percentage value
  soilMoisturePercent = map(soilMoistureValue, DryValue, WetValue, 0, 100);

  // Keep values between 0 and 100
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

  // Print raw value to serial monitor for sensor calibration
  Serial.println(soilMoistureValue);

  // Pass soil moisture to cloud variable
  current_Moisture = soilMoisturePercent;

  // See if pump should be triggered
  // See if moisture is below or equal to threshold
  if (soilMoisturePercent <= pump_trigger) {
    // Turn pump on
    pumpOn();

  } else {
    // Turn pump off
    pumpOff();
  }
  
  // Cycle values on OLED Display
  // Pump Status
  printOLED(35, "PUMP", 40, pump_status_text, 2000);
  // Temperature
  printOLED(35, "TEMP", 10, String(temp) + "C", 2000);
  // Humidity
  printOLED(30, "HUMID", 10, String(hum) + "%", 2000);
  // Moisture
  printOLED(35, "MOIST", 30, String(soilMoisturePercent) + "%", 2000);
 

}

void pumpOn() {
  // Turn pump on
  digitalWrite(RELAY_OUT, HIGH);
  pump_status_text = "ON";
  pump_Status = true;

}

void pumpOff() {
  // Turn pump off
  digitalWrite(RELAY_OUT, LOW);
  pump_status_text = "OFF";
  pump_Status = false;

}

void printOLED(int top_cursor, String top_text, int main_cursor, String main_text, int delay_time){
  // Prints to OLED and holds display for delay_time
  display.setCursor(top_cursor, 0);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println(top_text);

  display.setCursor(main_cursor, 40);
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.print(main_text);
  display.display();

  delay(delay_time);
  display.clearDisplay();
 
}


void onTriggerLevelChange()  {
  // Changes when Pump Trigger Level slider is activated

  pump_trigger = trigger_Level;

}