/*
Ésta versión, anula la necesidad de utilizar el Arduino de control del sensor de intensidad
(no he anulado su control, sigue siendo compatible con el)

  Este código es una implementación para Arduino Nano Due
  utilizando una pantalla SSD1309 y un módulo CAN
  (Controller Area Network SN65HVD230). El código controla y muestra información sobre el
  estado de la batería de alto voltaje sustituida en un Toyota Prius Gen 2.
  Realiza comunicación a través del bus CAN. Consiguiendo su optimización de funcionamiento.

    Al Prius NO LE GUSTA lo siguiente:

Voltaje real de la batería a 175 V o menos (independientemente de lo que le diga que es el voltaje). 
O bien, decirle al automóvil que el voltaje de la batería es mucho más alto de lo que realmente es
(por ejemplo, la batería es de 170 V, pero le dices que es de 200 V)
se genera un código de error, el coche se bloquea, en ese caso hay que 
borrar los fallos, decirle el voltaje correcto y todo estará bien.
Sí le dices al coche, que la batería está al 80 % del estado de carga o más
el motor funciona todo el tiempo, extrayendo corriente de la batería
(aproximadamente 9 A) para descargarla.
Resistencia de HV- o HV+ a la tierra del chasis inferior a unos 10 m

El Prius depende del motor en marcha para la distribución de aceite lubricante en los engranajes de la CVT 
Lo que implica que, una conversión enchufable debe garantizar que el Prius reinicie
el motor cada pocos kilómetros y luego vuelva a funcionar como vehículo eléctrico.

  https://www.eaa-phev.org/wiki/Prius_PHEV_TechInfo#4
  https://attachments.priuschat.com/attachment-files/2021/09/211662_Prius22009_CAnCodes.pdf
  https://en.wikipedia.org/wiki/OBD-II_PIDs#Standard_PIDs
  https://es.wikipedia.org/wiki/OBD-II_PID
  https://www.eaa-phev.org/wiki/Prius_PHEV_TechInfo#4
  https://github.com/Lorevalles/Proyecto_Prius
*/
// Mensajes que transmite el BMS (El BMS no recibe nada)
// 8 ms   03B      5 00 05 00 E2 27 (226V 0A) Itensidad y voltaje 03B  5 00 CB 00 E3 EE  (227V +20A) 5 0F 0F 00 E2 40 (226V -24A)
// 100 ms 3C9      8 03 FF 25 02 9A 03 22 BC 0x21 ( carga con potencia )
// 100 ms 3CB      7 67 64 00 99 15 13 61    SOC
// 100 ms 3CD      5 00 00 00 E1 B6          Voltaje del paquete: 16 bits, sin signo [V] SOC
// 100 ms 4D1      8 11 00 01 02 00 00 00 00 Datos desconocidos e inmutables. (A.V.: Batt -> HECU)


#include <U8g2lib.h>
#include <due_can.h>

U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/5, /* data=*/6, /* cs=*/7, /* dc=*/9, /* reset=*/8);

uint32_t receivedFrames;
CAN_FRAME incoming;

// Variable para almacenar el último mensaje recibido
uint16_t ultimo03BNeutro = 0xFFFF;          // Neutro
uint16_t ultimoMensajeRecibidoID = 0xFFFF;  // Inicializar con un valor que no sea un ID válido
uint16_t ultimo3C9 = 0xFFFF;                // Carga con potencia
uint16_t ultimo3CD = 0xFFFF;                // Voltaje paquete
uint16_t ultimoInmutables = 0xFFFF;         // Inmutable

//revoluciones
int rpmValor = 0;

float hvSocValue = 0;  // Representado

///unsigned long lastAvisoRvTime = 0;
const unsigned long avisoRvInterval = 1000;  // Intervalo de tiempo en milisegundos (1 segundo)
unsigned long lastAvisoRcTime = 0;
const unsigned long avisoRcInterval = 1000;  // Intervalo de tiempo en milisegundos (1 segundo)

unsigned long currentMillisEv = millis();  // Obtener el tiempo actual
unsigned long ultimaActualizacion = 0;     // Variable para almacenar el tiempo de la última actualización
int valorAnterior = -1;                    // Inicialmente, ningún valor anterior

// Declarar una variable para almacenar el tiempo del último cambio en el valor de lecturas por segundo (L:)
unsigned long lastReadingsUpdateMillis = 0;
// Declarar una variable para almacenar el tiempo del último cambio en el valor de mensajes enviados por segundo (E:)
unsigned long lastSentMessagesUpdateMillis = 0;

// Variables globales Control activaEv
bool activacionPendiente = false;
unsigned long ultimaActivacion = 0;
unsigned long tiempoUltimaAplicacion = 0;
unsigned long tiempoSinDatos = 0;
const unsigned int evTiactivado = 300;     // 300 milisegundos Tiempo pulsado el boton
const unsigned int tiempoEsperar = 30000;  // 30 segundos en milisegundos Ciclo de verificación

// Inicio encendido control
int tiempoEspera = 1000;  // Espera para poder apagar el vehiculo al reiniciar

// Valores control A0 A1 y D3, D4
const byte socBajo = 70;         //70 Valor a mantener minimo de SOC
const byte socAlto = 79;         //79 Valor a mantener Maximo de SOC
const byte limiteBajoSOC = 18;   //18 Limite para el SOC más bajo
const byte limiteCarga = 80;     //80 Comienza a descargar
const byte voltajeMinimo = 224;  // 224
const byte voltajeMaximo = 239;  // 240
const byte marcaBaja = 70;       // 70 Este valor tambien condiciona a partir de carga voltajeMaximo
const byte marcaAlta = 80;       // 80 Marca en icono SOC
const byte repMensaje = 2;       // 1 Número repeticiones enviado el mensaje.

// Variables para el control de la salida A0 (Rele Voltaje)
unsigned long lastOutputA0ChangeMillis = 0;  // Último cambio de estado
const unsigned int tiempoActivoA0 = 0;   // 15 segundos en milisegundos
const unsigned int tiempoInactivoA0 = 0;  // 5 segundos en milisegundos

// Refresco de pantalla
const unsigned int esperaRef = 1000;  // 1000 cada segundo actualiza los datos en la pantalla.

// Definición de pines para salidas
const byte outputPinD3 = 3;   // Carga
const byte outputPinD4 = 4;   // Descarga
const byte outputPinA0 = A0;  // Rele control voltaje simulador batería
const byte outputPinA1 = A1;  // Rele activación EV

const int botonPin = A2;        // Pin del botón en la entrada A2
const int interruptorPin = A3;  // Pin del interruptor en la entrada A3
// Declarar variables globales para mantener el estado anterior de A4 y A5
int lastA4State = HIGH;
int lastA5State = HIGH;
unsigned long lastAvisoRvTime = 0;  // Variable para realizar un seguimiento del tiempo del último avisoRv

// const int debounceDelay = 500;  // Retardo de rebote en milisegundos

int lastButtonState = LOW;           // Estado anterior del botón
int lastSwitchState = LOW;           // Estado anterior del interruptor
unsigned long lastDebounceTime = 0;  // Último tiempo de rebote registrado

// Definir las coordenadas para ubicar el ícono de la batería en la pantalla OLED
const uint8_t batteryIconX = 0;
const uint8_t batteryIconY = 0;
const uint8_t batteryIconWidth = 128;
const uint8_t batteryIconHeight = 8;

// Variables para mostrar el estado en la pantalla
float lastHvSocValue = 0;
int lastByte3 = 228;
int PAValor = 0;

// Declara una variable global para almacenar el estado de EV
String EV = "SI";  // Inicialmente, asumimos que EV es "SI"
unsigned long tiempoInicio = 0;

// Otras variables
bool outputA0Activated = false;
bool d3Activated = false;
bool lastD3Activated = false;
bool lcdNeedsUpdate = false;
unsigned int readingsPerSecond = 0;
unsigned int readCountPerSecond = 0;
unsigned int sentMessageCount = 0;
unsigned int sentMessagesPerSecond = 0;
unsigned int totalSentMessages = 0;

int decimalValue = 0;  // Acelerador posicon

// Control de tiempo y recuento
unsigned int readCount = 0;

// Variables para el control de tiempo y ciclos por segundo
unsigned long previousSecondMillis = 0;

void setup() {
  //Serial.begin(115200);

  if (Can0.begin(CAN_BPS_500K)) {
    //Serial.println("CAN0 inicializacion correcta");

    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 20, "CAN0 inicio correcto");
    u8g2.sendBuffer();
  } else {
    //Serial.println("CAN0 inicio ERROR");

    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.drawStr(0, 55, "CAN0 inicializacion error");
    u8g2.sendBuffer();
  }

  delay(tiempoEspera);  // Esta línea se ejecutará después del if-else.

  error();  // llamada para borrar errores

  //Can0.watchFor();  // Anula filtro

  // Configura los filtros para los ID de interés (SOLO LEE LOS PRIMEROS 7)

  Can0.watchFor(0x3C8);  // ID 0x3C8, DLC 5  (256 * C + D)   ICE RPM Actua
  Can0.watchFor(0x3CB);  // ID 0x3CB, DLC 7, SOC Batería
  Can0.watchFor(0x03B);  // ID 0x03B, DLC 5, Voltaje batería EV
  Can0.watchFor(0x529);  // ID 0x529, DLC 7, Estado EV (botón)
  Can0.watchFor(0x3CA);  // ID 0x3CA, DLC 5, Velocidad
  Can0.watchFor(0x244);  // ID 0x244, DLC 8, Acelerarador
  Can0.watchFor(0x3CD);  // ID 0x3CD, DLC 5, Voltaje del paquete: 16 bits, sin signo [V] SOC

  // Can0.watchFor(0x3C9);  // ID 0x3C9, DLC 8, ( carga con potencia )
  // Can0.watchFor(0x4D1);  // ID 0x4D1, DLC 8, Datos desconocidos e inmutables. (A.V.: Batt -> HECU)

  // Can0.watchFor(0x07E221C4);  // ID 0x07E221C4, DLC 8, Ángulo acelerador
  // Can0.watchFor(0x18DB33F1);  // ID 0x18DB33F1, DLC 8, Pregunta conexion 7DF equivalente
  // Can0.watchFor(0x039);  // ID 0x039 , DLC 4, ICE Temperature (A) 4 30 02 0D 7C (de 0 a 255 °C)
  // Can0.watchFor(0x52C);  // ID 0x52C , DLC 2, ICE Temperature (B/2) 2 23 60 (de 0 a 127 °C)
  // Can0.watchFor(0x5A4);  // ID 0x5A4 , DLC 2, Fuel Tank Level Measured (B) 2 63 11 (full tank 0x2C/44)

  u8g2.begin();
  do {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(5, 20, "Ciclo de espera");
    u8g2.setCursor(6, 40);
    u8g2.drawLine(6, 35, 120, 35);
    u8g2.drawStr(0, 55, "CAN0 verificado");
    u8g2.sendBuffer();
  } while (u8g2.nextPage());

  // Configura las entradas y salidas
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);
  pinMode(botonPin, INPUT_PULLUP);
  pinMode(interruptorPin, INPUT_PULLUP);
  pinMode(outputPinD3, OUTPUT);
  pinMode(outputPinD4, OUTPUT);
  pinMode(outputPinA0, OUTPUT);
  pinMode(outputPinA1, OUTPUT);

  // Inicializa las salidas
  digitalWrite(outputPinD3, LOW);
  digitalWrite(outputPinD4, LOW);
  digitalWrite(outputPinA0, HIGH);
  digitalWrite(outputPinA1, LOW);

  // Esperar tiempoEspera antes de continuar
  delay(tiempoEspera);
  u8g2.clearBuffer();
  u8g2.firstPage();
  u8g2.clearDisplay();
  u8g2.setFont(u8g2_font_6x10_tf);
  updateu8g2();
}

void sendCANMessage(uint16_t id, uint8_t dlc, uint8_t data[8]) {
  CAN_FRAME frame;
  frame.id = id;  // Utiliza directamente el ID de 11 bits
  frame.length = dlc;
  frame.extended = false;  // Asegura que estás utilizando mensajes de 11 bits

  for (int i = 0; i < dlc; i++) {
    frame.data.byte[i] = data[i];
  }

  Can0.sendFrame(frame);
}

void error() {  // Borra los códigos de error del vehiculo (Toyota Prius)
  // Mensaje CAN ID 7DF (11 bits), DLC 8, Datos 02 01 00 00 00 00 00 00
  uint8_t data1[] = { 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  sendCANMessage(0x7DF, 8, data1);
  sentMessageCount++;
  // Mensaje CAN ID 7E0 (11 bits), DLC 8, Datos 02 3E 00 00 00 00 00 00
  uint8_t data2[] = { 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  sendCANMessage(0x7E0, 8, data2);
  sentMessageCount++;
  // Mensaje CAN ID 7E2 (11 bits), DLC 8, Datos 02 3E 00 00 00 00 00 00
  uint8_t data3[] = { 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  sendCANMessage(0x7E2, 8, data3);
  sentMessageCount++;
  // Mensaje CAN ID 7E3 (11 bits), DLC 8, Datos 02 3E 00 00 00 00 00 00
  uint8_t data4[] = { 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  sendCANMessage(0x7E3, 8, data4);
  sentMessageCount++;
  // Mensaje CAN ID 7E8 (11 bits), DLC 8, Datos 01 44 00 00 00 00 00 00
  uint8_t data5[] = { 0x01, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  sendCANMessage(0x7E8, 8, data5);
  sentMessageCount++;
}

void mensajeSOC() {
  // Mensaje CAN ID 3CB (11 bits), DLC 7, Datos 69(CDL) 7D(CCL) 00(DELTA SOC) 93(SOC) 21(TEM1) 20(TEM2) 8F (ChkSum)
  // uint8_t data6[] = { 0x69, 0x7D, 0x00, 0x92, 0x21, 0x20, 0x8F };//soc 73
  // uint8_t data6[] = { 0x67, 0x64, 0x00, 0x99, 0x15, 0x13, 0x61 }; //soc 76,5
  uint8_t data6[] = { 0x69, 0x7D, 0x00, 0x93, 0x21, 0x20, 0x8F };  //soc 73,5
  for (int i = 0; i < repMensaje; i++) {                           // Número repeticiones enviando el mensaje.
    // Envía el mensaje
    sendCANMessage(0x3CB, 7, data6);
    sentMessageCount++;
  }
}

void mensajeCarga() {

  // Mensaje CAN ID 03B (11 bits), DLC 5 0F 0F 00 E2 40
  uint8_t data10[] = { 0x0F, 0x0F, 0x00, 0xE2, 0x40 };

  // Verificar si se ha recibido un mensaje en el ID deseado antes de enviar
  if (ultimoMensajeRecibidoID == 0x03B) {
    for (int i = 0; i < repMensaje; i++) {

      // Envía el mensaje
      sendCANMessage(0x03B, 5, data10);
      sentMessageCount++;
      ultimoMensajeRecibidoID = 0xFFFF;
    }
  }
}

void mensajeDescarga() {

  // Mensaje CAN ID 03B (11 bits), DLC 5 00 C9 00 E2 EB  (226V +20A)
  uint8_t data11[] = { 0x00, 0xC9, 0x00, 0xE2, 0xEB };

  // Verificar si se ha recibido un mensaje en el ID deseado antes de enviar
  if (ultimoMensajeRecibidoID == 0x03B) {
    for (int i = 0; i < repMensaje; i++) {  // Número repeticiones enviando el mensaje.

      // Envía el mensaje
      sendCANMessage(0x03B, 5, data11);
      sentMessageCount++;
      ultimoMensajeRecibidoID = 0xFFFF;
    }
  }
}

void mensajeNeutro() {

  // Mensaje CAN ID 03B (11 bits), DLC 5 00 05 00 E2 27  (226V +0A)
  uint8_t data8[] = { 0x00, 0x05, 0x00, 0xE2, 0x27 };

  // Verificar si se ha recibido un mensaje en el ID deseado antes de enviar
  if (ultimo03BNeutro == 0x03B) {
    for (int i = 0; i < repMensaje; i++) {  // Número repeticiones enviando el mensaje.

      // Envía el mensaje
      sendCANMessage(0x03B, 5, data8);
      sentMessageCount++;
      ultimo03BNeutro = 0xFFFF;
    }
  }
}

void voltajePaquete() {

  // Mensaje CAN ID 3CD (11 bits), 5 00 00 00 E1 B6
  uint8_t data9[] = { 0x00, 0x00, 0x00, 0xE1, 0xB6 };

  // Verificar si se ha recibido un mensaje en el ID deseado antes de enviar
  if (ultimo3CD == 0x3CD) {
    for (int i = 0; i < repMensaje; i++) {  // Número repeticiones enviando el mensaje.

      // Envía el mensaje
      sendCANMessage(0x3CD, 5, data9);
      sentMessageCount++;
      ultimo3CD = 0xFFFF;
    }
  }
}

void cargaPotencia() {

  // Mensaje CAN ID 3C9 (11 bits), 8 03 FF 25 02 9A 03 22 BC
  uint8_t data12[] = { 0x03, 0xFF, 0x25, 0x02, 0x9A, 0x03, 0x22, 0xBC };

  // Verifica si el mensaje recibido tiene el ID 0x3C9
  if (ultimo3C9 == 0x3C9) {
    for (int i = 0; i < repMensaje; i++) {  //repMensaje Número repeticiones enviando el mensaje.

      // Enviar el mensaje de respuesta inmediata
      sendCANMessage(0x3C9, 8, data12);
      sentMessageCount++;
      ultimo3C9 = 0xFFFF;
    }
  }
}

void datosInmutables() {
  // Mensaje CAN ID 4D1 (11 bits), 8 11 00 01 02 00 00 00 00 (A.V.: Batt -> HECU)
  uint8_t data7[] = { 0x11, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00 };

  // Verifica si el mensaje recibido tiene el ID 0x4D1
  if (ultimoInmutables == 0x4D1) {
    for (int i = 0; i < repMensaje; i++) {  // Número de repeticiones enviando el mensaje.

      // Envía el mensaje
      sendCANMessage(0x4D1, 8, data7);
      sentMessageCount++;
      ultimoInmutables = 0xFFFF;  // Actualiza después de enviar el mensaje la cantidad deseada de veces
    }
  }
}

void avisoRv() {  // Aviso rele control voltaje batería
  // Verifica si ha pasado al menos 1 segundo desde el último avisoRv
  unsigned long currentMillis = millis();
  if (currentMillis - lastAvisoRvTime >= avisoRvInterval) {
    // Realiza las acciones de avisoRv aquí
    u8g2.setCursor(83, 54);
    u8g2.print("V:M");
    u8g2.sendBuffer();

    // Actualiza el tiempo del último avisoRv
    lastAvisoRvTime = currentMillis;
  }
}

void avisoRc() {
  // Verifica si ha pasado al menos 1 segundo desde el último avisoRc
  unsigned long currentMillis = millis();
  if (currentMillis - lastAvisoRcTime >= avisoRcInterval) {
    // Realiza las acciones de avisoRc aquí
    u8g2.setCursor(110, 54);
    u8g2.print("C:M");
    u8g2.sendBuffer();

    // Actualiza el tiempo del último avisoRc
    lastAvisoRcTime = currentMillis;
  }
}

void controla() {  // Control por pantalla(BANDERA)
  u8g2.setCursor(63, 54);
  u8g2.print(lastHvSocValue);
  u8g2.sendBuffer();
  delay(500);
}

void control(int value) {  // Control por pantalla(BANDERA)
  u8g2.setCursor(63, 54);
  u8g2.print(decimalValue);
  u8g2.sendBuffer();
  //delay(1000);
}

void drawCurrentIcon(uint8_t y, int value) {
  const uint8_t maxCurrent = 100;  // Valor máximo de corriente
  const uint8_t barHeight = 9;     // Altura de la barra
  const uint8_t barSpacing = 2;    // Espacio entre barras

  // Calcular el ancho de la barra
  int barWidth = map(abs(value), 0, maxCurrent, 0, 128);

  // Dibujar la barra
  u8g2.drawFrame(0, 35, 128, barHeight);
  if (value >= 0) {
    u8g2.drawBox(0, 35, barWidth, barHeight - 1);
  } else {
    u8g2.drawBox(128 - barWidth, 35, barWidth, barHeight - 1);
  }
}

void drawThrottleBar(uint8_t decimalValue) {
  const int maxAcele = 200;   // Valor máximo de corriente
  const int baraHeight = 6;   // Altura de la barra
  const int baraSpacing = 1;  // Espacio entre barras
  const int baraX = 0;        // Posición X de la barra
  const int baraY = 19;       // Posición Y de la barra
  const int baraWidth = 128 - baraX * 2;

  // Mapea el valor de entrada al ancho de la barra
  int baraLength = map(abs(decimalValue), 0, maxAcele, 0, baraWidth);

  // Dibuja el marco de la barra
  u8g2.drawFrame(baraX, baraY, baraWidth, baraHeight);

  // Dibuja la barra de acuerdo al valor de aceleración
  if (decimalValue >= 0) {
    u8g2.drawBox(baraX, baraY, baraLength, baraHeight - 1);
  } else {
    u8g2.drawBox(baraX + baraWidth - baraLength, baraY, baraLength, baraHeight - 1);
  }
}

// Función para dibujar el ícono SOC
void drawBatteryIcon(uint8_t percentage) {
  // Calcular el ancho total del ícono proporcional al valor del porcentaje
  uint8_t iconWidth = map(percentage, 0, 100, 0, batteryIconWidth);

  // Dibujar el rectángulo que representa la barra de SOC
  u8g2.drawFrame(batteryIconX, batteryIconY, batteryIconWidth, batteryIconHeight);

  // Dibujar las marcas en las posiciones 60% y 80% dentro del ícono de la batería
  uint8_t mark60 = map(marcaBaja, 0, 100, 0, batteryIconWidth);
  uint8_t mark80 = map(marcaAlta, 0, 100, 0, batteryIconWidth);
  u8g2.drawLine(batteryIconX + mark60, batteryIconY, batteryIconX + mark60, batteryIconY + batteryIconHeight);
  u8g2.drawLine(batteryIconX + mark80, batteryIconY, batteryIconX + mark80, batteryIconY + batteryIconHeight);

  // Rellenar el rectángulo que representa la barra de SOC hasta el valor correspondiente
  u8g2.drawBox(batteryIconX, batteryIconY, iconWidth, batteryIconHeight);
}

void updateu8g2() {
  u8g2.clearBuffer();
  //  Llamar a la función para dibujar el ícono del acelerador
  drawThrottleBar(decimalValue);
  // Llamar a la función para dibujar el ícono SOC de la batería con el valor actual de porcentaje
  drawBatteryIcon(hvSocValue);

  //Revoluciones Rpm.
  u8g2.setCursor(71, 64);
  u8g2.print("RPM:");
  u8g2.print(rpmValor);

  // Mostrar el porcentaje de carga de la batería en la posición (100, 10) de la pantalla LCD
  u8g2.setCursor(0, 18);
  u8g2.print("SOC:");
  u8g2.print(hvSocValue, 1);
  u8g2.print("%");

  // Mostrar la intensidad de corriente en la posición (0, 33) de la pantalla LCD
  u8g2.setCursor(0, 33);
  u8g2.print("A:");
  u8g2.print(PAValor);
  drawCurrentIcon(21, PAValor);

  // Mostrar el voltaje en la posición (92, 33) de la pantalla LCD
  u8g2.setCursor(72, 33);
  u8g2.print("V:");
  u8g2.print(lastByte3);

  // Mostrar el valor de EV en la posición (50, 33) de la pantalla LCD
  u8g2.setCursor(38, 33);
  u8g2.print("EV:");
  u8g2.print(EV);

  // Mostrar el estado de la salida A0 (R.Bat.H o R.Bat.L) en la posición (80, 54) de la pantalla LCD
  u8g2.setCursor(110, 33);
  if (outputA0Activated) {
    u8g2.print("R.H");
  } else {
    u8g2.print("R.L");
  }

  // Mostrar si el SOC Esta o no en rango
  u8g2.setCursor(90, 18);
  if (hvSocValue <= marcaBaja || hvSocValue >= marcaAlta) {
    u8g2.print("No");
  } else {
    u8g2.print("Si");
  }

  // Mostrar si se puede parar o no en la posición (0, 64) de la pantalla LCD

  if (d3Activated) {
    u8g2.setCursor(0, 64);
    u8g2.print("NOP");

  } else {
    digitalWrite(outputPinD3, LOW);
    u8g2.setCursor(0, 64);
    u8g2.print("SIP");
  }

  //Mostrar los valores de lecturas y envíos por segundo en las posiciones (0, 54) y (30, 54) de la pantalla LCD
  u8g2.setCursor(0, 54);
  u8g2.print("L:");
  u8g2.print(readingsPerSecond);

  // Mostrar el valor de mensajes enviados por segundo (E:) en la posición (38, 54) de la pantalla LCD
  u8g2.setCursor(38, 54);
  u8g2.print("E:");
  u8g2.print(sentMessagesPerSecond);

  int switchState = digitalRead(interruptorPin);
  // Controlar el estado de la salida D4 y mostrarlo en la posición (60, 64) de la pantalla LCD
  if (lastHvSocValue >= limiteCarga && (!switchState == HIGH)) {
    digitalWrite(outputPinD4, HIGH);
    u8g2.setCursor(26, 64);
    u8g2.print("De.A");
  } else {
    digitalWrite(outputPinD4, LOW);
    u8g2.setCursor(26, 64);
    u8g2.print("De.I");
  }

  // Intensidad barra -----
  u8g2.setCursor(0, 43);
  if (PAValor >= 0) {
    int lineLength = map(PAValor, 0, 100, 0, 20);
    for (int i = 0; i < 20; i++) {
      if (i >= 20 - lineLength) {
        u8g2.print("+");
      } else {
        u8g2.print(" ");
      }
    }
  } else {
    int lineLength = map(PAValor, 0, -100, 0, 20);
    for (int i = 0; i < 20; i++) {
      if (i < lineLength) {
        u8g2.print("-");
      } else {
        u8g2.print(" ");
      }
    }
  }
  u8g2.sendBuffer();  // Enviar los datos a la pantalla LCD
}

// Declaración de la función para actualizar el estado de la salida A0
void updateOutputA0(bool state) {
  digitalWrite(A0, state ? HIGH : LOW);
}
void processCanMessage(CAN_FRAME& incoming) {
  readCount++;
  readCountPerSecond++;


  // Revoluciones
  if (incoming.id == 0x3C8 && incoming.length == 5) {  // Ajustado el número de bytes a 8
    byte byte2 = incoming.data.byte[2];
    byte byte3 = incoming.data.byte[3];

    rpmValor = ((byte2 * 256) + byte3) / 8;
  }

  // SOC
  if (incoming.id == 0x3CB && incoming.length == 7) {
    byte byte2 = incoming.data.byte[2];
    byte byte3 = incoming.data.byte[3];
    hvSocValue = ((byte2 * 256 + byte3) / 2.0);

    // Forzar envío
    if ((incoming.id == 0x3CB && digitalRead(interruptorPin) == HIGH) && hvSocValue >= limiteBajoSOC) {
      mensajeSOC();
    } else
      // Verifica si el valor es mayor a socAlto o si se cumplen las condiciones originales
      if (((incoming.id == 0x3CB && (hvSocValue < marcaBaja || hvSocValue > marcaAlta)) || hvSocValue > socAlto) && hvSocValue >= limiteBajoSOC) {
        mensajeSOC();
      }

    if (hvSocValue != lastHvSocValue) {
      lastHvSocValue = hvSocValue;
    }
  }

  // Voltios
  if (incoming.id == 0x03B && incoming.length == 5) {
    byte byte1 = incoming.data.byte[1];
    byte byte3 = incoming.data.byte[3];
    ultimoMensajeRecibidoID = 0x03B;
    int hvV = (incoming.data.byte[2] * 256) + byte3;
    hvV = (hvV & 0x07FF) - (hvV & 0x0800);
    hvV *= 1;
    // Amperios
    int aValue = ((incoming.data.byte[0]) * 256) + (incoming.data.byte[1]);
    if ((aValue & 0x800) != 0) {
      aValue = aValue - 0x1000;
    }
    PAValor = aValue / 10;

    lastByte3 = hvV;
  }

  // Acelerador posicion
  if (incoming.id == 0x244 && incoming.length == 8) {
    byte byte6 = incoming.data.byte[6];

    // Convertir valor hexadecimal a decimal
    decimalValue = byte6;
  }
}

void leerCan() {
  // Variables locales para almacenar mensajes CAN recibidos
  CAN_FRAME incoming;

  // Comprueba si hay datos CAN recibidos
  if (Can0.available()) {
    Can0.read(incoming);

    // Llama a tu función de procesamiento para procesar el mensaje CAN
    activaEv(incoming);  // Pasa el objeto CAN_FRAME como argumento

    // Verifica si el mensaje cumple con alguna de las condiciones
    if ((incoming.id == 0x3CB && incoming.length == 7) || (incoming.id == 0x03B && incoming.length == 5) || (incoming.id == 0x529 && incoming.length == 7) || (incoming.id == 0x3CA && incoming.length == 5) || (incoming.id == 0x244 && incoming.length == 8) || (incoming.id == 0x3C9 && incoming.length == 8) || (incoming.id == 0x3CD && incoming.length == 5) || (incoming.id == 0x4D1 && incoming.length == 8) || (incoming.id == 0x3C8 && incoming.length == 5)) {

      // Llama a tu función de procesamiento para procesar el mensaje CAN
      processCanMessage(incoming);
    }
  }
}

void activaEv(CAN_FRAME& canMsg) {
  unsigned long currentMillisEv = millis();

  // Calcular el tiempo desde la última recepción de datos
  tiempoSinDatos = currentMillisEv - tiempoUltimaAplicacion;

  if (canMsg.id == 0x529) {
    if (canMsg.data.byte[4] == 0x40) {
      // Si can_id es 0x529 y data[4] es 0x40, establece el mensaje en "SI"
      EV = "SI";
    } else {
      // Si can_id es 0x529 pero data[4] no es 0x40, establece el mensaje en "NO"
      EV = "NO";
    }
  }

  if (EV == "NO") {
    // Verificar la velocidad del vehículo
    if (canMsg.id == 0x3CA && activacionPendiente == false && canMsg.data.byte[2] <= 0x2D && hvSocValue >= 50) {
      // Verificar si la velocidad es igual o inferior a 45 km/h en hexadecimal
      if (activacionPendiente || tiempoSinDatos >= tiempoEsperar) {
        // Si hay una activación pendiente o ha pasado el tiempo de espera,
        // inicia la activación y guarda el tiempo
        ultimaActivacion = currentMillisEv;
        activacionPendiente = true;
        digitalWrite(outputPinA1, HIGH);  // Activa el relé
      }
    }
  }

  if (activacionPendiente && currentMillisEv - ultimaActivacion >= evTiactivado) {
    // Si ha pasado el tiempo de activación, establece en LOW
    digitalWrite(outputPinA1, LOW);            // Establece en LOW
    activacionPendiente = false;               // Marca que no hay una activación pendiente
    tiempoUltimaAplicacion = currentMillisEv;  // Actualiza el tiempo de última aplicación
  }
}

void conCarga() {
  unsigned long currentMillis = millis();
  unsigned long elapsedTime = currentMillis - lastOutputA0ChangeMillis;

  // Variable para realizar un seguimiento del último estado de salida A0
  static bool lastOutputA0State = false;

  if (lastHvSocValue <= marcaBaja) {
    // Si lastHvSocValue es menor o igual a marcaBaja, DESACTIVAR la salida A0

    if (!lastOutputA0State && elapsedTime >= tiempoInactivoA0) {
      outputA0Activated = false;  // Cambiado a false
      updateOutputA0(outputA0Activated);
      lastOutputA0ChangeMillis = currentMillis;
      lastOutputA0State = true;  // Mantener este true, ya que indica que se realizó un cambio
    }
  } else if ((lastByte3 <= voltajeMinimo) && (hvSocValue <= socBajo)) {
    // Si lastHvSocValue no cumple la primera condición pero lastByte3 es menor o igual a voltajeMinimo
    if (!lastOutputA0State && elapsedTime >= tiempoInactivoA0) {
      outputA0Activated = false;  // Cambiado a false
      updateOutputA0(outputA0Activated);
      lastOutputA0ChangeMillis = currentMillis;
      lastOutputA0State = true;  // Mantener este true, ya que indica que se realizó un cambio
    }
  } else if (lastByte3 >= voltajeMaximo) {
    // Si ninguna de las condiciones anteriores se cumple pero lastByte3 es mayor o igual a voltajeMaximo
    if (lastOutputA0State && elapsedTime >= tiempoActivoA0) {
      outputA0Activated = true;  // Cambiado a true
      updateOutputA0(outputA0Activated);
      lastOutputA0ChangeMillis = currentMillis;
      lastOutputA0State = false;  // Mantener este false, ya que indica que se realizó un cambio
    }
  } else {
    // Si ninguna de las condiciones anteriores se cumple, restablecer el estado de salida A0 a su valor anterior
    if (lastOutputA0State && elapsedTime >= tiempoActivoA0) {
      outputA0Activated = true;  // Cambiado a true
      updateOutputA0(outputA0Activated);
      lastOutputA0ChangeMillis = currentMillis;
      lastOutputA0State = false;  // Mantener este false, ya que indica que se realizó un cambio
    }
  }
}


void lecSegundo() {  // Actualizar el recuento de mensajes enviados por esperaRef
  unsigned long currentMillislec = millis();
  if (currentMillislec - previousSecondMillis >= esperaRef) {
    previousSecondMillis = currentMillislec;

    readingsPerSecond = readCountPerSecond;  // Guardar el valor actual de readCountPerSecond en readingsPerSecond
    readCountPerSecond = 0;                  // Reiniciar readCountPerSecond para contar el próximo segundo

    // Actualizar valores de lecturas y mensajes enviados en el LCD
    sentMessagesPerSecond = sentMessageCount;
    if (lcdNeedsUpdate) updateu8g2();

    sentMessageCount = 0;  // Reiniciar el contador de mensajes enviados por segundo
    lcdNeedsUpdate = true;
  }
}

void controlSoc() {
  // Control SOC
  if (lastHvSocValue <= marcaBaja) {
    d3Activated = true;
    digitalWrite(outputPinD3, HIGH); // Relé control voltaje batería 
    /*
    // Variable local para almacenar mensajes CAN recibidos
    CAN_FRAME incoming;
*/

    // Bucle mientras haya datos CAN disponibles
    while (Can0.available()) {
      Can0.read(incoming);


      // Procesar mensajes con ID 0x3C9
      if (incoming.id == 0x3C9 && incoming.length == 8) {
        ultimo3C9 = 0x3C9;
        //cargaPotencia();
      } else if (incoming.id == 0x03B && incoming.length == 5) {
        ultimoMensajeRecibidoID = 0x03B;
        //mensajeCarga();
      }
      // Procesar mensajes con ID 0x3CD
      else if (incoming.id == 0x3CD && incoming.length == 5) {
        ultimo3CD = 0x3CD;
        voltajePaquete();
      }
      // Procesar mensajes con ID 0x4D1
      else if (incoming.id == 0x4D1 && incoming.length == 8) {
        ultimoInmutables = 0x4D1;
        datosInmutables();
      }
    }
  }

  // Agrega esta condición para que d3Activated no pueda ser false en otros casos
  else if (lastByte3 >= voltajeMaximo && lastHvSocValue >= marcaBaja) {
    d3Activated = false;
    digitalWrite(outputPinD3, LOW);
  }

  // Define una variable para indicar si la condición se cumple
  bool condicionCumplida = false;

  // Establece la duración máxima del ciclo en milisegundos
  unsigned long duracionMaxima = 1;  // Por ejemplo, 1 milisegundos afecta a CAN velocidad

  // Registra el tiempo de inicio
  unsigned long tiempoInicio = millis();

  while (!condicionCumplida) {
    // Verifica si la condición lastByte3 >= voltajeMaximo todavía se cumple

    if (lastByte3 >= voltajeMaximo) {
      // Si la condición aún se cumple, verifica si ha pasado el tiempo máximo
      if (millis() - tiempoInicio >= duracionMaxima) {
        condicionCumplida = true;  // Si ha pasado el tiempo máximo, sale del bucle
      }
    } else {
      // Si la condición ya no se cumple, detén el bucle
      condicionCumplida = true;
      cargaDes();
    }
  }
}
void cargaDes() {
  // Controlar la salida D3 dependiendo del valor de SOC y lastHvSocValue
  if (lastHvSocValue <= socBajo) {
    if (!d3Activated) {
      digitalWrite(outputPinD3, HIGH);  // Activar D3
      d3Activated = true;
      lcdNeedsUpdate = true;
    }
  } else if (lastHvSocValue >= socAlto) {
    if (d3Activated) {
      digitalWrite(outputPinD3, LOW);  // Desactivar D3
      d3Activated = false;
      lcdNeedsUpdate = true;
    }
  }
}
//////////////////////////////////////////////////////////////////////////

void loop() {
  // Lee el estado actual del botón, el interruptor y las entradas A4 y A5
  int buttonState = digitalRead(botonPin);
  int switchState = digitalRead(interruptorPin);
  int a4State = digitalRead(A4);
  int a5State = digitalRead(A5);

  // Comprueba si A4 o A5 han cambiado de estado
  if (a4State != lastA4State || a5State != lastA5State) {
    // Si alguno de ellos ha cambiado, ejecuta avisoRv()
  }

  // Actualiza el estado anterior de A4 y A5
  lastA4State = a4State;
  lastA5State = a5State;

  // Verifica si A4 o A5 están en estado de activación
  if (a4State == LOW || a5State == LOW) {
    // Si al menos uno de ellos está en estado de activación, establece el estado de pinA0
    digitalWrite(outputPinA0, (a4State == HIGH) ? HIGH : LOW);
    avisoRv();
  }

  // Comprueba si el botón se ha pulsado
  if (buttonState == LOW && lastButtonState == HIGH) {
    // Borra el error
    error();
  }
  /*
  // Comprueba si el interruptor está en la posición correcta y queda condicionado por Rpm
  if (switchState == HIGH || rpmValor > 1) {
    avisoRc();
    */
  // Parpadea C:M en la pantalla
  if (switchState == HIGH) {
    avisoRc();

    // Verifica que al menos uno de los pines D3 o D4 esté alto
    if (digitalRead(outputPinD3) == HIGH || digitalRead(outputPinD4) == HIGH) {
      d3Activated = false;
      lastHvSocValue = socAlto + 1;
      // Cambia los pines D3 y D4 a bajo
      digitalWrite(outputPinD3, LOW);
      // digitalWrite(outputPinD4, LOW);
    }
  }

  // Actualiza el estado anterior del botón y el interruptor
  lastButtonState = buttonState;
  lastSwitchState = switchState;

  /////////////////////////////////////////////////////////////////////

  leerCan();           // Llama a leerCan() en el bucle principal
  activaEv(incoming);  // Pasa el objeto CAN_FRAME como argumento
  conCarga();
  lecSegundo();
  controlSoc();
}
