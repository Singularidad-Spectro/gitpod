#include <Servo.h>

#include <Servo.h>

/*
 Controlling a servo position using a potentiometer (variable resistor)
 by Michal Rinott <http://people.interaction-ivrea.it/m.rinott>

 modified on 8 Nov 2013
 by Scott Fitzgerald
 http://www.arduino.cc/en/Tutorial/Knob
*/

#include <Servo.h>

Servo myservo;  // create servo object to control a servo
Servo servoMotor; 
int potpin = A0;  // analog pin used to connect the potentiometer
int val;    // variable to read the value from the analog pin

void setup() {
  myservo.attach(9);  
    servoMotor.attach(2);// attaches the servo on pin 2 to the servo object
}

void loop() {

//COMUNICACION CON ESP8266
Serial.println("Hello Boss I'm Arduino Leonardo");
  delay(1500);

  val = analogRead(potpin);            // reads the value of the potentiometer (value between 0 and 1023)
  val = map(val, 0, 150, 0, 180);     // scale it for use with the servo (value between 0 and 180)
  myservo.write(val);                  // sets the servo position according to the scaled value
  delay(0.5);     
  
  // Desplazamos a la posición 0º
  servoMotor.write(0);
  delay(1000);  // Esperamos 1 segundo
  
  // Desplazamos a la posición 90º
  servoMotor.write(90);
  delay(1000);  // Esperamos 1 segundo
  
  // Desplazamos a la posición 180º
  servoMotor.write(180);
  delay(1000);  // Esperamos 1 segundo                      // waits for the servo to get there
}
