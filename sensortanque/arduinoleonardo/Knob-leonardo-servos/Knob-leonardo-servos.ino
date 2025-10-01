#include <Servo.h>

Servo servoHorario;    // Servo para seguimiento horario
Servo servoInclinacion;  // Servo para inclinación
String inputString = "";
boolean stringComplete = false;

void setup() {
  Serial.begin(9600);  // Iniciar comunicación serial
  servoHorario.attach(9);      // Servo horario en pin 9
  servoInclinacion.attach(2);   // Servo inclinación en pin 2
  
  // Posición inicial
  servoHorario.write(0);
  servoInclinacion.write(0);
  
  inputString.reserve(200);
}

void loop() {
  // Procesar comando cuando esté completo
  if (stringComplete) {
    processCommand(inputString);
    inputString = "";
    stringComplete = false;
  }
}

// Evento de recepción serial
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
}

// Procesar comandos recibidos
void processCommand(String command) {
  // Formato: "H:angle" para horario, "I:angle" para inclinación
  if (command.length() > 2 && command.charAt(1) == ':') {
    char type = command.charAt(0);
    int angle = command.substring(2).toInt();
    
    // Asegurar que el ángulo esté en rango
    angle = constrain(angle, 0, 180);
    
    if (type == 'H') {
      servoHorario.write(angle);
      Serial.print("Horario a ");
      Serial.print(angle);
      Serial.println(" grados");
    }
    else if (type == 'I') {
      servoInclinacion.write(angle);
      Serial.print("Inclinacion a ");
      Serial.print(angle);
      Serial.println(" grados");
    }
  }
}
