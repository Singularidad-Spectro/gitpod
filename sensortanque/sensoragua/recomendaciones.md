# Recomendaciones y Esquema de Conexión

## Tabla de Conexiones (con HM Level Converter)

| Dispositivo         | Pin/Color         | Conexión física                | Destino/Función                | Notas clave                       |
|---------------------|-------------------|-------------------------------|-------------------------------|-----------------------------------|
| **Sensor de nivel** | Rojo              | +12 V                         | Fuente externa 12 V           | No conectar al ESP/NodeMCU        |
|                     | Verde/Negro       | GND                           | GND fuente 12 V y NodeMCU     | Masa común                        |
|                     | Amarillo          | Señal                         | Un extremo de resistencia shunt|                                   |
| **Resistencia shunt**| -                | 150 Ω (o 47 Ω si usas A0)     | Entre Amarillo y GND          | Adapta corriente a voltaje        |
| **NodeMCU (ESP8266)**| A0               | Nodo Amarillo (señal)         | Entrada analógica (ADC)       | Lee voltaje convertido            |
|                     | GND               | GND común                     | Sensor y fuente 12 V          |                                   |
|                     | D6                | TRIGGER HC-SR04               | Sensor ultrasónico            |                                   |
|                     | D7                | ECHO HC-SR04                  | Sensor ultrasónico            |                                   |
|                     | D2                | TFT_DC (GC9A01A)              | Pantalla reloj                | Según diagrama de la página       |
|                     | D8                | TFT_CS (GC9A01A)              | Pantalla reloj                | Según diagrama de la página       |
| **HM Level Converter** | HV              | 5 V Leonardo                  | Alimentación alto voltaje     |                                   |
|                        | LV              | 3.3 V ESP8266                 | Alimentación bajo voltaje     |                                   |
|                        | GND             | GND común                     | ESP8266 y Arduino Leonardo    |                                   |
|                        | HV1             | TX Arduino Leonardo (pin 1)   | Entrada canal 1 alto voltaje  |                                   |
|                        | LV1             | RX ESP8266 (GPIO3)            | Salida canal 1 bajo voltaje   |                                   |
|                        | HV2             | RX Arduino Leonardo (pin 0)   | Entrada canal 2 alto voltaje  |                                   |
|                        | LV2             | TX ESP8266 (GPIO1)            | Salida canal 2 bajo voltaje   |                                   |
| **Arduino Leonardo**| Pin 9             | Servo horario                 | Control de posición           |                                   |
|                     | Pin 2             | Servo inclinación             | Control de inclinación        |                                   |
|                     | Pin 0 (RX)        | HV2 del Level Converter       | Comunicación serial           |                                   |
|                     | Pin 1 (TX)        | HV1 del Level Converter       | Comunicación serial           |                                   |
| **Servos**          | VCC               | 5 V Leonardo                  | Alimentación servos           | Mejor si fuente externa           |
|                     | GND               | GND Leonardo                  | Alimentación servos           |                                   |
| **Pantalla GC9A01A**| Según diagrama    | D2, D8, etc. NodeMCU          | Reloj circular                | Ver página de referencia          |

---

## Esquema de Conexión (texto)

```
Panel Solar (+12 V) ──> Sensor Rojo
Panel Solar (GND) ──┬─> Sensor Verde/Negro
                    └─> GND ESP8266 (masa común)
Sensor Amarillo ──> [Resistencia Shunt] ──> GND
                │
                └─> A0 ESP8266

Pila de litio (+3.7 V regulada a 3.3 V) ──> VCC ESP8266
Pila de litio (GND) ──> GND ESP8266 (unida a masa común)

ESP8266 TX/RX ──> HM Level Converter LV2/LV1 ──> Leonardo RX/TX (HV2/HV1)
HM Level Converter GND ──> Masa común

Leonardo 5 V ──> HV del Level Converter
ESP8266 3.3 V ──> LV del Level Converter

Servos ──> Leonardo (pines 9 y 2, alimentados por 5 V Leonardo)
Pantalla GC9A01A ──> ESP8266 (pines D2, D8, etc.)
```

---

## Recomendaciones clave

- Nunca conectes +12 V al ESP8266.
- Masa común entre todos los sistemas.
- El ESP8266 solo recibe la señal analógica adaptada por la resistencia.
- El HM Level Converter protege la comunicación serial entre ESP8266 y Leonardo.
- Calibra el sistema midiendo el voltaje con el tanque vacío y lleno, y usa esos valores en el código.
- Si usas NodeMCU, verifica si tu placa ya tiene divisor interno para A0 (algunas admiten hasta 3.3 V, otras solo 1 V).
- Alimenta el ESP8266 con pila de litio regulada a 3.3 V, nunca con 12 V.
- Los servos deben alimentarse desde el Leonardo o fuente externa de 5 V.
- Consulta el diagrama de la página para la pantalla GC9A01A.

---

¿Necesitas agregar más detalles o ejemplos de código?