#include <SPI.h>
#include <mcp2515.h>
#include <avr/wdt.h>

struct can_frame canMsg;
MCP2515 mcp2515(10);

unsigned long previousMillis = 0;
const unsigned long interval = 10;

void setup() {
  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  // Configurar perro guardián (watchdog) para reiniciar el Arduino en caso de bloqueo
  wdt_enable(WDTO_2S);
}

void loop() {
  unsigned long currentMillis = millis();

  // Verificar si ha pasado el intervalo de tiempo para enviar el siguiente mensaje
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    canMsg.can_id = 0x3CB;
    canMsg.can_dlc = 7;
    canMsg.data[0] = 0x69;
    canMsg.data[1] = 0x7D;
    canMsg.data[2] = 0x00;
    canMsg.data[3] = 0x89;
    canMsg.data[4] = 0x1E;
    canMsg.data[5] = 0x1B;
    canMsg.data[6] = 0x7D;

    mcp2515.sendMessage(&canMsg);
  }

  // Alimentar el perro guardián (watchdog)
  wdt_reset();
}
