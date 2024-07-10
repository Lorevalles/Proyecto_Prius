#include <avr/wdt.h> // Wacthdog.Por si se cuelga reinicia.
#include <CD74HC4067.h>

const int inputPins[] = {10, 11, 12, 9};
CD74HC4067 mux1(13, 8, 7, 6);
CD74HC4067 mux2(5, 4, 3, 2);

const int waitTime = 1;
const int debounceDelay = 400; // Tiempo de debounce en milisegundos, en reaccioner a D9...D12.
const int PIN_D12 = 0b0001;
const int PIN_D9 = 0b0010;
const int PIN_D10 = 0b0100;
const int PIN_D11 = 0b1000;

unsigned long lastDebounceTime = 0;
byte lastInputPinsValue = 0;
unsigned long previousMillis = 0;
unsigned long previousReadMillis = 0;
const unsigned long readInterval = 500; // Tiempo de espera entre lecturas en milisegundos

void setup() {
  //Serial.begin(9600);
  for (int i = 0; i < 4; i++) {
    pinMode(inputPins[i], INPUT_PULLUP);
  }
  mux1.channel(16);
  mux2.channel(0);
  wdt_enable(WDTO_2S); // Habilitar watchdog con tiempo de espera de 2 segundos
}

void loop() {
  wdt_reset(); // Reiniciar el temporizador del watchdog en cada iteración del bucle

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= waitTime) {
    previousMillis = currentMillis;

    byte inputPinsValue = 0;
    for (int i = 0; i < 4; i++) {
      inputPinsValue |= (!digitalRead(inputPins[i])) << i; // Invertir la lógica
    }

    if ((currentMillis - lastDebounceTime) > debounceDelay) {
      lastDebounceTime = currentMillis;
      if (inputPinsValue != lastInputPinsValue) {
        lastInputPinsValue = inputPinsValue;
        if (inputPinsValue == 0) { // Si no hay entrada en ningún pin, activar el canal 16 mux1 y 0 mux2
          mux1.channel(16);
          mux2.channel(0);
          //Serial.println("Seleccionando canal 16 mux1 y mux2 0 ...");
        } else {
          if (inputPinsValue & PIN_D12) {
            mux1.channel(0);
            mux2.channel(0);
           //Serial.println("Seleccionando canal 0 del Mux-1...");
          } else if (inputPinsValue & PIN_D9) {
            mux1.channel(1);
            mux2.channel(0);
            //Serial.println("Seleccionando canal 1 del Mux-1...");
          } else if (inputPinsValue & PIN_D10) {
            mux2.channel(1);
            mux1.channel(16);
            //Serial.println("Seleccionando canal 1 del Mux-2...");
          } else if (inputPinsValue & PIN_D11) {
            mux2.channel(2);
            mux1.channel(16);
            //Serial.println("Seleccionando canal 2 del Mux-2...");
          }
        }
      }
    }

    // Verificar el tiempo transcurrido desde la última lectura de A0
    if (currentMillis - previousReadMillis >= readInterval) {
      previousReadMillis = currentMillis;

      int analogValue = analogRead(A0);
      //Serial.print("Valor leido A0: ");
      //Serial.println(analogValue);

      // Verificar si el valor está en el rango de 110 a 390
      if (analogValue >= 220 && analogValue <= 280) {
        //Serial.println("Dentro del rango");
        {
          if (inputPinsValue & PIN_D12) {
            mux1.channel(0);
            mux2.channel(0);
            //Serial.println("Seleccionando canal 0 del Mux-1...");
          } else if (inputPinsValue & PIN_D9) {
            mux1.channel(1);
            mux2.channel(0);
            //Serial.println("Seleccionando canal 1 del Mux-1...");
          } else if (inputPinsValue & PIN_D10) {
            mux2.channel(1);
            mux1.channel(16);
            //Serial.println("Seleccionando canal 1 del Mux-2...");
          } else if (inputPinsValue & PIN_D11) {
            mux2.channel(2);
            mux1.channel(16);
            //Serial.println("Seleccionando canal 2 del Mux-2...");
          }
        }
      } else {
        if (analogValue < 200 || analogValue > 300) {
          mux1.channel(16);
          mux2.channel(0);
          //Serial.println("Fuera de rango, anula ordenes de D9...D12");
        }
      }
    }
  }
}
