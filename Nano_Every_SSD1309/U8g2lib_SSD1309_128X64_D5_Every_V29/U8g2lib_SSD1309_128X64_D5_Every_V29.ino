/*
  Este código es una implementación para Arduino Nano Every
  utilizando una pantalla u8g2 y un módulo CAN
  (Controller Area Network). El código controla y muestra información sobre el
  estado de la batería de alto voltaje sustituida en un Toyota Prius Gen 2
  y realiza comunicación a través del bus CAN. Consiguiendo su optimización de funcionamiento.

  https://github.com/Lorevalles/Proyecto_Prius
*/
#include <U8g2lib.h>
#include <SPI.h>
#include <mcp2515.h>
#include <Wire.h>

//mcp2515 conexiones
const int SPI_CS_PIN = 10;
const int INT_PIN = 2;
const int SS_RX_PIN = 11;
const int SS_TX_PIN = 12;

// Declarar una variable global para almacenar el valor anterior del SOC
uint8_t previousSocValue = 0;

U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/5, /* data=*/6, /* cs=*/7, /* dc=*/9, /* reset=*/8);
struct can_frame canMsg;
MCP2515 mcp2515(SPI_CS_PIN);

int tiempoEspera = 5000;
// Declarar una variable para almacenar el tiempo del último cambio en el valor de lecturas por segundo (L:)
unsigned long lastReadingsUpdateMillis = 0;
// Declarar una variable para almacenar el tiempo del último cambio en el valor de mensajes enviados por segundo (E:)
unsigned long lastSentMessagesUpdateMillis = 0;

// Declarar la variable messageCountPerSecond al comienzo del programa
unsigned int messageCountPerSecond = 0;

// Definir el tiempo de inactividad en milisegundos antes de considerar el valor como 0
const unsigned long inactivityThreshold = 2000;

// Definir las coordenadas para ubicar el ícono de la batería en la pantalla LCD
const uint8_t batteryIconX = 2;
const uint8_t batteryIconY = 2;
const uint8_t batteryIconWidth = 124;
const uint8_t batteryIconHeight = 12;

// Variables para el control de inactividad y mensaje de "Sin datos"
const unsigned long noDataTimeout = 5000;  // Tiempo límite de inactividad en milisegundos (por ejemplo, 5 segundos)
unsigned long lastDataUpdateTime = 0;      // Variable para almacenar el tiempo de la última actualización de datos

// Variables para mostrar el estado en la pantalla
float lastHvSocValue = 0.0;
int lastByte3 = 0;
int amperiosValor = 0;
int EV = 0;

// Otras variables
bool outputD14Activated = false;
bool d3Activated = false;
bool lastD3Activated = false;
bool lcdNeedsUpdate = false;
bool waiting = true;
unsigned long waitStartTime = 0;
unsigned int readingsPerSecond = 0;
unsigned int readCountPerSecond = 0;
unsigned int sentMessageCount = 0;
unsigned int sentMessagesPerSecond = 0;
unsigned int totalSentMessages = 0;
unsigned int messagesSentThisSecond = 0;
// Declarar una variable para almacenar el tiempo de inicio del segundo actual
unsigned long currentSecondStart = 0;
// Declarar una variable para almacenar el tiempo del último segundo
unsigned long lastSecondMillis = 0;
// Control de tiempo y recuento
unsigned int readCount = 0;

// Definición de pines para salidas
const int outputPinD3 = 3;
const int outputPinD4 = 4;
const int outputPinA0 = A0;

// Variables para el control de tiempo
const unsigned long waitDuration = 5000;  // Duración de espera inicial en milisegundos (5 segundos)
unsigned long lastMessageSentTime = 0;
unsigned long previousMessageMillis = 0;
unsigned long previousLcdUpdateMillis = 0;
const unsigned long lcdUpdateInterval = 1000;  // Intervalo de actualización de la pantalla LCD (500 milisegundos)
const unsigned long messageInterval = 108;     // Intervalo de envío de mensaje (108 milisegundos)

// Variables para el control de tiempo y ciclos por segundo
unsigned long previousSecondMillis = 0;

unsigned long previousMessageSecond = 0;  // Variable para almacenar el segundo anterior

// Variable para indicar si los datos de SOC se han actualizado o no
bool socDataUpdated = false;

// Para activar el modo EV cada 30 segundos si el modo EV
unsigned long previousEvActivationTime = 0;
const unsigned long evActivationInterval = 30000;  // Intervalo de 30 segundos para activar el modo EV
int dados_f[1];                                    // Declaración de la variable dados_f como un arreglo de enteros con tamaño 1

// Definir una variable global para almacenar el último valor de porcentaje SOC
uint8_t lastPercentage = 0;

// Indicador de mensaje válido recibido en el bus CAN
bool message_ok = false;

void initializeDisplay() {
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(5, 20, "Ciclo de espera");
  u8g2.setCursor(6, 40);
  u8g2.drawLine(6, 35, 120, 35);
  u8g2.drawStr(0, 55, "Se puede PARAR");
  u8g2.setFontRefHeightExtendedText();
  u8g2.setFontDirection(0);
  u8g2.sendBuffer();
}
void setup() {
  u8g2.begin();
  initializeDisplay();

  // Inicialización de las salidas digitales
  pinMode(outputPinD3, OUTPUT);
  pinMode(outputPinD4, OUTPUT);
  pinMode(outputPinA0, OUTPUT);
  digitalWrite(outputPinD3, HIGH);
  digitalWrite(outputPinD4, HIGH);
  digitalWrite(outputPinA0, LOW);


  // Esperar tiempoEspera antes de continuar
  delay(tiempoEspera);
  u8g2.clearDisplay();
  u8g2.setFont(u8g2_font_6x10_tf);

  // Inicialización de la comunicación SPI para el módulo CAN
  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  // Enviar el mensaje activateEVMode al inicio del programa
  activateEVMode();
  // Reinicia errores comienzo
  errorMotor();
  errorHybrid();
  errorBattery();
  finalObd();
}

void drawCurrentIcon(uint8_t y, int value) {
  const uint8_t maxCurrent = 100;  // Valor máximo de corriente
  const uint8_t barHeight = 10;    // Altura de la barra
  const uint8_t barSpacing = 2;    // Espacio entre barras

  // Calcular el ancho de la barra
  int barWidth = map(abs(value), 0, maxCurrent, 0, 128);

  // Dibujar la barra en el búfer
  u8g2.clearBuffer();
  u8g2.drawFrame(0, 35, 128, barHeight);
  if (value >= 0) {
    u8g2.drawBox(0, 35, barWidth, barHeight - 1);
  } else {
    u8g2.drawBox(128 - barWidth, 35, barWidth, barHeight - 1);
  }

  // Enviar el búfer a la pantalla usando el bucle do-while
  u8g2.sendBuffer();
}

// Función para actualizar la salida D14 y mostrar el estado en la pantalla LCD
void updateOutputD14(bool activated) {
  digitalWrite(outputPinA0, activated ? HIGH : LOW);
}

// Enviar solicitudes al sistema de diagnóstico del vehículo.
void finalObd() {
  canMsg.can_id = 0x7DF;
  canMsg.can_dlc = 8;
  canMsg.data[0] = 0x02;
  canMsg.data[1] = 0x01;
  canMsg.data[2] = 0x00;
  canMsg.data[3] = 0x00;
  canMsg.data[4] = 0x00;
  canMsg.data[5] = 0x00;
  canMsg.data[6] = 0x00;
  canMsg.data[7] = 0x00;

  mcp2515.sendMessage(&canMsg);
  sentMessageCount++;
}

void inicioObd1() {
  canMsg.can_id = 0x7DF;  // ID del mensaje de solicitud de diagnóstico (Diagnostics Request)
  canMsg.can_dlc = 8;
  canMsg.data[0] = 0x02;  // Tamaño del mensaje (solicitud de PID)
  canMsg.data[1] = 0x09;  // Código del PID específico para obtener VIN
  canMsg.data[2] = 0x00;  // Datos adicionales (si es necesario)
  canMsg.data[3] = 0x00;
  canMsg.data[4] = 0x00;
  canMsg.data[5] = 0x00;
  canMsg.data[6] = 0x00;
  canMsg.data[7] = 0x00;

  mcp2515.sendMessage(&canMsg);
  sentMessageCount++;
}

// OBD-II inicio?
void inicioObd() {
  canMsg.can_id = 0x18DB33F1;
  canMsg.can_dlc = 8;
  canMsg.data[0] = 0x02;
  canMsg.data[1] = 0x01;
  canMsg.data[2] = 0x00;
  canMsg.data[3] = 0x00;
  canMsg.data[4] = 0x00;
  canMsg.data[5] = 0x00;
  canMsg.data[6] = 0x00;
  canMsg.data[7] = 0x00;

  mcp2515.sendMessage(&canMsg);
  sentMessageCount++;
}

// Borra error de Unidad de control del motor #1
void errorMotor() {
  if (EV == 0) {  // Solo activar el modo EV si está desactivado
    canMsg.can_id = 0x7E0;
    canMsg.can_dlc = 8;
    canMsg.data[0] = 0x02;
    canMsg.data[1] = 0x3E;
    canMsg.data[2] = 0x00;
    canMsg.data[3] = 0x00;
    canMsg.data[4] = 0x00;
    canMsg.data[5] = 0x00;
    canMsg.data[6] = 0x00;
    canMsg.data[7] = 0x00;

    mcp2515.sendMessage(&canMsg);
    sentMessageCount++;
  }
}
// Borra error de Hybrid engine system
void errorHybrid() {
  if (EV == 0) {  // Solo activar el modo EV si está desactivado
    canMsg.can_id = 0x7E2;
    canMsg.can_dlc = 8;
    canMsg.data[0] = 0x02;
    canMsg.data[1] = 0x3E;
    canMsg.data[2] = 0x00;
    canMsg.data[3] = 0x00;
    canMsg.data[4] = 0x00;
    canMsg.data[5] = 0x00;
    canMsg.data[6] = 0x00;
    canMsg.data[7] = 0x00;

    mcp2515.sendMessage(&canMsg);
    sentMessageCount++;
  }
}
// Borra error de HV battery
void errorBattery() {
  if (EV == 0) {  // Solo activar el modo EV si está desactivado
    canMsg.can_id = 0x7E3;
    canMsg.can_dlc = 8;
    canMsg.data[0] = 0x02;
    canMsg.data[1] = 0x3E;
    canMsg.data[2] = 0x00;
    canMsg.data[3] = 0x00;
    canMsg.data[4] = 0x00;
    canMsg.data[5] = 0x00;
    canMsg.data[6] = 0x00;
    canMsg.data[7] = 0x00;

    mcp2515.sendMessage(&canMsg);
    sentMessageCount++;
  }
}
void activateEVMode() {
  if (EV == 0) {  // Solo activar el modo EV si está desactivado
    canMsg.can_id = 0x529;
    canMsg.can_dlc = 5;
    canMsg.data[0] = 0x00;
    canMsg.data[1] = 0x00;
    canMsg.data[2] = 0x00;
    canMsg.data[3] = 0x00;
    canMsg.data[4] = 0x01;  // Valor 1 para activar el modo EV

    mcp2515.sendMessage(&canMsg);
    sentMessageCount++;
  }
}
void deactivateEVMode() {
  if (EV == 1) {  // Solo desactivar el modo EV si está activado
    canMsg.can_id = 0x529;
    canMsg.can_dlc = 5;
    canMsg.data[0] = 0x00;
    canMsg.data[1] = 0x00;
    canMsg.data[2] = 0x00;
    canMsg.data[3] = 0x00;
    canMsg.data[4] = 0x00;  // Valor 0 para desactivar el modo EV

    mcp2515.sendMessage(&canMsg);
    sentMessageCount++;
  }
}
// Función para dibujar el ícono de la batería en la pantalla LCD SOC
void drawBatteryIcon(uint8_t percentage) {
  // Calcular el ancho total del ícono proporcional al valor del porcentaje
  uint8_t iconWidth = map(percentage, 0, 100, 0, batteryIconWidth);
  // Inicializar el proceso de dibujo de la página
  u8g2.firstPage();

  // Dibujar en el búfer de la página actual
  u8g2.clearBuffer();

  // Dibujar el rectángulo que representa la barra de SOC
  u8g2.drawFrame(batteryIconX, batteryIconY, batteryIconWidth, batteryIconHeight);

  // Dibujar las marcas en las posiciones 60% y 80% dentro del ícono de la batería
  uint8_t mark60 = map(60, 0, 100, 0, batteryIconWidth);
  uint8_t mark80 = map(80, 0, 100, 0, batteryIconWidth);
  u8g2.drawLine(batteryIconX + mark60, batteryIconY, batteryIconX + mark60, batteryIconY + batteryIconHeight);
  u8g2.drawLine(batteryIconX + mark80, batteryIconY, batteryIconX + mark80, batteryIconY + batteryIconHeight);

  // Rellenar el rectángulo que representa la barra de SOC hasta el valor correspondiente
  u8g2.drawBox(batteryIconX, batteryIconY, iconWidth, batteryIconHeight);

  // Enviar el búfer a la pantalla
  u8g2.sendBuffer();
}

void updateDisplay() {
  // Llama a la función para dibujar elementos estáticos
  drawStaticElements();
  // Leer el valor actual del SOC desde lastHvSocValue
  uint8_t currentSocValue = lastHvSocValue;

  // Comprobar si el valor del SOC ha cambiado
  if (currentSocValue != previousSocValue) {
    // Actualizar solo cuando el SOC cambie
    lcdNeedsUpdate = true;
    drawBatteryIcon(currentSocValue);
    previousSocValue = currentSocValue;
  }
}

void drawStaticElements() {
  // Dibuja el icono del SOC en la misma página
  drawBatteryIcon(lastHvSocValue);
}

void drawDynamicElements() {
  u8g2.firstPage();
  // Dibuja otros elementos en la misma página
  u8g2.setCursor(0, 24);
  u8g2.print("SOC:");
  u8g2.print(lastHvSocValue, 1);
  u8g2.print("%");

  u8g2.setCursor(0, 33);
  u8g2.print("A:");
  u8g2.print(amperiosValor);
  drawCurrentIcon(21, amperiosValor);

  u8g2.setCursor(92, 33);
  u8g2.print("V:");
  u8g2.print(lastByte3);

  // Mostrar el valor de EV en la posición (50, 33) de la pantalla LCD
  u8g2.setCursor(50, 33);
  u8g2.print("EV:");
  u8g2.print(EV);

  // Mostrar el estado de la salida D14 (R.Bat.H o R.Bat.L) en la posición (80, 54) de la pantalla LCD
  u8g2.setCursor(80, 54);
  if (outputD14Activated) {
    u8g2.print("R.Bat.H");
  } else {
    u8g2.print("R.Bat.L");
  }

  // Mostrar si el SOC está o no en rango
  if (lastHvSocValue < 60 || lastHvSocValue > 79) {
    u8g2.setCursor(75, 24);
    u8g2.print("Fuera");
  } else {
    u8g2.setCursor(75, 24);
    u8g2.print("Entro");
  }

  // Mostrar los valores de lecturas y envíos por segundo en las posiciones (0, 54) y (38, 54) de la pantalla LCD
  u8g2.setCursor(0, 54);
  u8g2.print("L:");
  u8g2.print(readingsPerSecond);

  u8g2.setCursor(38, 54);
  u8g2.print("E:");
  u8g2.print(sentMessagesPerSecond);

  // Mostrar si se debe parar o no en la posición (0, 64) de la pantalla LCD
  if (d3Activated) {
    u8g2.setCursor(0, 64);
    u8g2.print("NO PARAR");
  } else {
    u8g2.setCursor(0, 64);
    u8g2.print("SI POWER");
  }

  // Controlar el estado de la salida D4 y mostrarlo en la posición (60, 64) de la pantalla LCD
  if (lastHvSocValue > 76) {
    digitalWrite(outputPinD4, LOW);
    u8g2.setCursor(60, 64);
    u8g2.print("Ac.Descarga");
  } else {
    digitalWrite(outputPinD4, HIGH);
    u8g2.setCursor(60, 64);
    u8g2.print("De.Inactiva");
  }

  // Intensidad barra -----
  u8g2.setCursor(0, 44);
  if (amperiosValor >= 0) {
    int lineLength = map(amperiosValor, 0, 100, 0, 20);
    for (int i = 0; i < 20; i++) {
      if (i >= 20 - lineLength) {
        u8g2.print("+");
      } else {
        u8g2.print(" ");
      }
    }
  } else {
    int lineLength = map(amperiosValor, 0, -100, 0, 20);
    for (int i = 0; i < 20; i++) {
      if (i < lineLength) {
        u8g2.print("-");
      } else {
        u8g2.print(" ");
      }
    }
  }

  // Marca la pantalla como actualizada
  lcdNeedsUpdate = false;
}

void updateu8g2() {
  // Borra la pantalla antes de redibujar
  u8g2.firstPage();
  u8g2.clearBuffer();
  // Llama a la función para dibujar elementos dinámicos
  drawDynamicElements();

  // Marca la pantalla como actualizada
  lcdNeedsUpdate = false;
}

// Declarar estructura para mensajes CAN
struct can_message {
  uint32_t can_id;  // ID del mensaje CAN
  uint8_t can_dlc;  // Longitud de datos del mensaje CAN
  uint8_t data[8];  // Datos del mensaje CAN
};
void processCanMessage() {
  readCount++;
  readCountPerSecond++;

  // SOC
  if (canMsg.can_id == 0x3CB && canMsg.can_dlc == 7) {
    byte byte2 = canMsg.data[2];
    byte byte3 = canMsg.data[3];
    float hvSocValue = ((byte2 * 256 + byte3) / 2.0);

    if (hvSocValue != lastHvSocValue) {
      lastHvSocValue = hvSocValue;
      lcdNeedsUpdate = true;
    }
  }
  // Voltios
  if (canMsg.can_id == 0x03B && canMsg.can_dlc == 5) {
    byte byte1 = canMsg.data[1];
    byte byte3 = canMsg.data[3];
    int hvV = (canMsg.data[2] * 256) + byte3;
    hvV = (hvV & 0x07FF) - (hvV & 0x0800);
    hvV *= 1;
    // Amperios
    int aValue = ((canMsg.data[0]) * 256) + (canMsg.data[1]);
    if ((aValue & 0x800) != 0) {
      aValue = aValue - 0x1000;
    }
    amperiosValor = aValue / 10;

    lastByte3 = hvV;
  }
  // EV
  if ((canMsg.can_id == 0x529 && !message_ok)) {
    dados_f[0] = canMsg.data[4];  // Leer el valor EV del mensaje CAN y almacenarlo en dados_f[0]
    message_ok = true;
    // Asignar el valor de dados_f[0] a la variable EV
    EV = dados_f[0];
    // Actualizar el tiempo de la última actualización de datos
    lastDataUpdateTime = millis();
  }
}
void loop() {
  unsigned long currentMillis = millis();

  // Leer mensajes CAN y procesarlos
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    processCanMessage();
    // Indicar que la pantalla LCD necesita actualizarse solo si hay cambios en los datos
    lcdNeedsUpdate = true;
  }

  // Enviar mensajes CAN a intervalos regulares
  if (currentMillis - previousMessageMillis >= messageInterval) {
    previousMessageMillis = currentMillis;

    // Envío del mensaje CAN dependiendo del valor de SOC
    if (lastHvSocValue < 60 || lastHvSocValue > 79) {
      canMsg.can_id = 0x3CB;
      canMsg.can_dlc = 7;
      canMsg.data[0] = 0x69;
      canMsg.data[1] = 0x7D;
      canMsg.data[2] = 0x00;
      canMsg.data[3] = 0x93;
      canMsg.data[4] = 0x21;
      canMsg.data[5] = 0x20;
      canMsg.data[6] = 0x8F;

      if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK) {
        sentMessageCount++;
        u8g2.setCursor(66, 54);
        u8g2.print("S");
      } else {
        u8g2.print("N");
      }
      u8g2.sendBuffer();  // Enviar los datos a la pantalla LCD
    }
  }

  // Controlar la salida D14 dependiendo del valor de voltaje
  if (lastByte3 <= 220) {
    // Si el valor es igual o menor a 220, activar la salida D14
    if (!outputD14Activated) {
      outputD14Activated = true;
      updateOutputD14(outputD14Activated);
    }
  } else if (lastByte3 >= 230) {
    // Si el valor es mayor o igual a 230, desactivar la salida D14
    if (outputD14Activated) {
      outputD14Activated = false;
      updateOutputD14(outputD14Activated);
    }
  }

  // Actualizar el recuento de mensajes enviados por segundo y lecturas por segundo
  if (currentMillis - previousSecondMillis >= 1000) {
    previousSecondMillis = currentMillis;

    readingsPerSecond = readCountPerSecond;  // Guardar el valor actual de readCountPerSecond en readingsPerSecond
    readCountPerSecond = 0;                  // Reiniciar readCountPerSecond para contar el próximo segundo

    // Actualizar valores de lecturas y mensajes enviados en el LCD
    sentMessagesPerSecond = sentMessageCount;

    sentMessageCount = 0;  // Reiniciar el contador de mensajes enviados por segundo
    lcdNeedsUpdate = true;
    // Actualizar la pantalla solo cuando sea necesario (cuando el SOC cambie)
    updateu8g2();
  }

  // Controlar la salida D3 dependiendo del valor de SOC
  if (lastHvSocValue <= 70 && !d3Activated) {
    digitalWrite(outputPinD3, LOW);
    d3Activated = true;
    lcdNeedsUpdate = true;  // Indicar que la pantalla LCD necesita actualizarse
  } else if (lastHvSocValue >= 74 && d3Activated) {
    digitalWrite(outputPinD3, HIGH);
    d3Activated = false;
    lcdNeedsUpdate = true;  // Indicar que la pantalla LCD necesita actualizarse
  } else if (lastHvSocValue > 70 && lastHvSocValue < 74) {
    if (d3Activated != lastD3Activated) {
      digitalWrite(outputPinD3, !d3Activated);
      lastD3Activated = d3Activated;
      lcdNeedsUpdate = true;  // Indicar que la pantalla LCD necesita actualizarse
    }
  }
}
