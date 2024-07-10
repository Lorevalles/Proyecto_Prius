#include <avr/wdt.h>
#include <SPI.h>
#include <mcp2515.h>
#include <CanHacker.h>
#include <CanHackerLineReader.h>

const int SPI_CS_PIN = 10;
const int INT_PIN = 2;

CanHackerLineReader *lineReader = NULL;
CanHacker *canHacker = NULL;

unsigned long previousMillis = 0;
const unsigned long interval = 100; // Intervalo de tiempo para procesar el perro guardián (100 ms)

void setup() {
  Serial.begin(115200);
  SPI.begin();

  canHacker = new CanHacker(&Serial, NULL, SPI_CS_PIN);
  lineReader = new CanHackerLineReader(canHacker);

  pinMode(INT_PIN, INPUT);

  // Configurar perro guardián (watchdog) para reiniciar el Arduino en caso de bloqueo
  wdt_enable(WDTO_2S); // Reinicio del Arduino si el perro guardián no se alimenta durante 2 segundos
}

void loop() {
  // Alimentar el perro guardián (watchdog)
  wdt_reset();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (digitalRead(INT_PIN) == LOW) {
      canHacker->processInterrupt();
    }

    if (Serial.available()) {
      lineReader->process();
    }
  }
}

void serialEvent() {
  lineReader->process();
}
