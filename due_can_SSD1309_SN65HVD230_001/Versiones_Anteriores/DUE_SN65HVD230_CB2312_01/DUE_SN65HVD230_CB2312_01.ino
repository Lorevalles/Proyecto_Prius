/*
Ésta versión, anula la necesidad de utilizar el Arduino de control del sensor de intensidad
(no he anulado su control, sigue siendo compatible con el)

  Este código es una implementación para Arduino Nano Due
  utilizando una pantalla SSD1309 y un módulo CAN
  (Controller Area Network SN65HVD230). El código controla y muestra información sobre el
  estado de la batería de alto voltaje sustituida en un Toyota Prius Gen 2.
  Realiza comunicación a través del bus CAN. Consiguiendo su optimización de funcionamiento.

ESTRUCTURA DE FUNCIONAMIENTO:

1º Comunicación CAN: Utiliza la librería due_can para interactuar con dispositivos a través del bus CAN, configurando filtros específicos para capturar solo mensajes de interés. Esto permite monitorear y controlar varios parámetros del vehículo como RPM del motor, temperatura, voltaje de la batería, etc.
2º Pantalla OLED: A través de la librería U8g2lib, el programa muestra información relevante en una pantalla OLED, incluyendo estado del CAN, SOC (estado de carga de la batería), energía consumida/regenerada, voltaje, corriente, etc. La pantalla también es utilizada para mostrar mensajes de error o de operación normal.
3º Control de Energía: El programa realiza un seguimiento de la energía consumida y regenerada, calculando estos valores en base al voltaje y la corriente medidos, y los muestra en la pantalla OLED.
4º Monitoreo de Voltaje y SOC: Implementa un sistema de control para monitorear y ajustar el voltaje y la carga de la batería basándose en varios umbrales predefinidos, como los niveles alto y bajo de SOC, y los voltajes mínimo y máximo.
5º Histéresis y Control de Mensajes: Controla el envío de mensajes basado en condiciones específicas utilizando un enfoque de histéresis para evitar fluctuaciones y asegurar estabilidad en las lecturas de SOC y voltaje.
6º Gestión de Dispositivos de Salida: Controla varios dispositivos de salida como relés y otros actuadores basados en las condiciones del vehículo detectadas a través de las entradas CAN. Esto incluye la activación de relés para gestionar el voltaje de acuerdo a los parámetros de carga de la batería.
7º Funcionalidad de Seguridad y Diagnóstico: Incluye funciones para borrar errores y manejar condiciones de error, así como monitorear el estado y la activación de componentes clave a través de pines específicos.
8º Interacción con el Usuario: Procesa entradas de botones e interruptores para realizar acciones como borrar errores o cambiar configuraciones, y muestra alertas visuales en la pantalla OLED como respuesta a estas interacciones.
9º Gestión del Tiempo y Eventos: Usa temporizadores para gestionar intervalos de actualización, monitoreo de condiciones a lo largo del tiempo, y para realizar acciones periódicas como el envío de mensajes o la actualización de la pantalla.

El código está estructurado para la gestión del Toyota Prius Gen-2 mediante la monitorización y control en tiempo real de varios parámetros críticos, ofreciendo una interfaz visual rica y funcionalidades de control automático basadas en los datos recibidos del bus CAN.
    
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

 Mensajes que transmite el BMS (El BMS no recibe nada)
 8 ms   03B      5 00 05 00 E2 27 (226V 0A) Itensidad y voltaje 03B  5 00 CB 00 E3 EE  (227V +20A) 5 0F 0F 00 E2 40 (226V -24A)
 100 ms 3C9      8 03 FF 25 02 9A 03 22 BC 0x21 ( carga con potencia )
 100 ms 3CB      7 67 64 00 99 15 13 61    SOC
 100 ms 3CD      5 00 00 00 E1 B6          Voltaje del paquete: 16 bits, sin signo [V] SOC
 100 ms 4D1      8 11 00 01 02 00 00 00 00 Datos desconocidos e inmutables. (A.V.: Batt -> HECU)
*/

#include <U8g2lib.h>
#include <due_can.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include "UTF8ToGB2312.h"

U8G2_SSD1322_240X128_1_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
//U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/5, /* data=*/6, /* cs=*/7, /* dc=*/9, /* reset=*/8);

uint32_t receivedFrames;
CAN_FRAME incoming;

int ultimoCargaModuloBatValido = 0;  // La primera vez, no tenemos un valor previo, puedes ajustar este inicio según sea necesario

// Variables globales para acumular energía
float energiaAcumulada = 0.0;              // Energía consumida en vatios-hora
float energiaRegenerada = 0.0;             // Energía regenerada en vatios-hora
unsigned long tiempoAnterior = 0;          // Para calcular el intervalo de tiempo
const long intervaloActualizacion = 1000;  // Intervalo de actualización de 1000 ms (1 segundo)
int cargaModuloBat = 0;                    // Declaración global

// Variable para histéresis enviando mensajes
bool permitirMensajeSOC = true;
// Define si el mensaje SOC está permitido basado en el voltajeV
bool permitirMensajePorVoltaje = true;  // o false, dependiendo del estado inicial deseado

//revoluciones
int rpmValor = 0;
int temperatura = 0;

float hvSocValue = 0;                      // Representado
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
const unsigned int tiempoEsperar = 15000;  // 30 segundos en milisegundos Ciclo de verificación

// Inicio encendido control
int tiempoEspera = 1000;  // Espera para poder apagar el vehiculo al reiniciar

// Valores control A0 A1 y D3, D4
const float factor = 1.089;        //Factor de multiplicaciónpor resistencia del simulador de batería
const byte socBajo = 70;           //70 Valor a mantener minimo de SOC. A partir de éste valor Relé ativado
const byte limiteBajoSOC = 25;     //20 Limite para el SOC más bajo
const byte histeresisAlto = 35;    //25 Histéresis SOC más bajo
const byte limiteCarga = 80;       //80 Dejará de enviar mensajes (Comienza a descargar)
const byte voltajeMinimo = 224;    // 224
const byte voltajeMaximo = 240;    // 240
const byte marcaBaja = 70;         // 70 Este valor tambien condiciona a partir de carga voltajeMaximo
const byte marcaAlta = 80;         // 80 Marca en icono SOC
const byte repMensaje = 2;         // 2 Número repeticiones enviado el mensaje.
const byte volLimBajoBat = 192;    // Limite de voltaje minimo de la batería para dejar de enviar mensaje SOC
const byte volLimHiteresis = 200;  // Hiteresis Limite de voltaje minimo
const byte mostrarNivelCarga = 5;  // Rango para mostrar actualizado el nivel de carga de la batería +-
bool displayOn = true;             // Controla si el valor se muestra o no en ese rango

// Variables para el control de la salida A0 (Rele Voltaje)
unsigned long lastOutputA0ChangeMillis = 0;  // Último cambio de estado
const unsigned int tiempoActivoA0 = 1000;    // 1 segundos en milisegundos
const unsigned int tiempoInactivoA0 = 1000;  // 15 segundos en milisegundos

// Refresco de pantalla
const unsigned int esperaRef = 100;  // 1000 cada segundo actualiza los datos en la pantalla.

// Definición de pines para salidas
const byte outputPinA0 = A0;  // Rele control voltaje simulador batería
const byte outputPinA1 = A1;  // Rele activación EV
// Definición de pines para entrada
const int botonPin = A2;    // Pin del botón en la entrada A2
int lastButtonState = LOW;  // Estado anterior del botón

// Definir las coordenadas para ubicar el ícono de la batería en la pantalla OLED
const uint8_t batteryIconX = 0;
const uint8_t batteryIconY = 0;
const uint8_t batteryIconWidth = 128;
const uint8_t batteryIconHeight = 8;

// Variables para mostrar el estado en la pantalla
float lastHvSocValue = 0;
int voltajeV = 228;
int voltajeTotal = 228;
float voltajeModulo = 3.72;
float PAValor = 0;

// Declara una variable global para almacenar el estado de EV
String EV = "SI";  // Inicialmente, asumimos que EV es "NO"

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
  // Inicialización de componentes, como en tu código original
  tiempoAnterior = millis();  // Guarda el tiempo inicial

  if (Can0.begin(CAN_BPS_500K)) {
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 20, "CAN0 inicio correcto");
    u8g2.sendBuffer();
  } else {
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.drawStr(0, 55, "CAN0 inicializacion error");
    u8g2.sendBuffer();
  }

  delay(tiempoEspera);  // Esta línea se ejecutará después del if-else.

  error();  // llamada para borrar errores

  //Can0.watchFor();  //Descomentada Anula filtro

  // Configura los filtros para los ID de interés (SOLO LEE LOS PRIMEROS 7)

  Can0.watchFor(0x3C8);  // ID 0x3C8, DLC 5  (256 * C + D)   ICE RPM Actua
  Can0.watchFor(0x3CB);  // ID 0x3CB, DLC 7, SOC Batería
  Can0.watchFor(0x03B);  // ID 0x03B, DLC 5, Voltaje batería EV
  Can0.watchFor(0x529);  // ID 0x529, DLC 7, Estado EV (botón)
  Can0.watchFor(0x3CA);  // ID 0x3CA, DLC 5, Velocidad
  Can0.watchFor(0x244);  // ID 0x244, DLC 8, Acelerarador
  Can0.watchFor(0x039);  // ID 0x039 , DLC 4, ICE Temperature (A) 4 30 02 0D 7C (de 0 a 255 °C)

  //Can0.watchFor(0x3CD);  // ID 0x3CD, DLC 5, Voltaje del paquete: 16 bits, sin signo [V] SOC
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
    u8g2.drawStr(0, 55, "Borrado errores");
    u8g2.sendBuffer();
  } while (u8g2.nextPage());

  // Configura las entradas y salidas
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);
  pinMode(botonPin, INPUT_PULLUP);
  pinMode(outputPinA0, OUTPUT);
  pinMode(outputPinA1, OUTPUT);

  // Inicializa las salidas
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
  // uint8_t data6[] = { 0x69, 0x7D, 0x00, 0x93, 0x21, 0x20, 0x8F };  //soc 73,5
  // uint8_t data6[] = { 0x67, 0x5B, 0x00, 0x9B, 0x15, 0x13, 0x5A }; //soc 77,5
  uint8_t data6[] = { 0x69, 0x6C, 0x00, 0x9F, 0x1B, 0x1A, 0x7E };  //soc 79,5

  for (int i = 0; i < repMensaje; i++) {  // Número repeticiones enviando el mensaje.
    // Envía el mensaje
    sendCANMessage(0x3CB, 7, data6);
    sentMessageCount++;
  }
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
  u8g2.drawBox(batteryIconX, batteryIconY, iconWidth, batteryIconHeight / 2.5);
}

// Inicializar cargaModuloBat
int calcularCargaModuloBat(float voltajeModulo) {
  if (voltajeModulo >= 4.2) return 100;
  if (voltajeModulo <= 3.61) return voltajeModulo < 3.5 ? 0 : 5;  // Asume 0% para voltajes < 3.5V

  // Puntos conocidos (voltaje, porcentaje)
  float puntos[][2] = {
    { 4.2, 100 }, { 4.15, 95 }, { 4.11, 90 }, { 4.08, 85 }, { 4.02, 80 }, { 3.98, 75 }, { 3.95, 70 }, { 3.91, 65 }, { 3.87, 60 }, { 3.85, 55 }, { 3.84, 50 }, { 3.82, 45 }, { 3.80, 40 }, { 3.79, 35 }, { 3.77, 30 }, { 3.75, 25 }, { 3.73, 20 }, { 3.71, 15 }, { 3.69, 10 }, { 3.61, 5 }
  };

  // Interpolación lineal entre puntos
  for (int i = 0; i < sizeof(puntos) / sizeof(puntos[0]) - 1; i++) {
    if (voltajeModulo >= puntos[i + 1][0] && voltajeModulo < puntos[i][0]) {
      // Interpolar entre puntos[i] y puntos[i+1]
      float m = (puntos[i][1] - puntos[i + 1][1]) / (puntos[i][0] - puntos[i + 1][0]);
      float b = puntos[i][1] - m * puntos[i][0];
      return static_cast<int>(m * voltajeModulo + b);
    }
  }

  return 0;  // Por defecto, si no se ajusta a ningún rango
}

void dibujarNivelCarga(int cargaModuloBat) {
  // Configuración de la barra de carga
  const int maxCarga = 100;                    // Valor máximo de carga en porcentaje
  const int alturaBarra = 4;                   // Altura de la barra de carga
  const int espaciadoBarra = 1;                // Espacio entre barras, si es necesario
  const int posXBarra = 0;                     // Posición X de la barra de carga
  const int posYBarra = 4;                     //u8g2.getDisplayHeight() - alturaBarra - espaciadoBarra;  // Posición Y de la barra de carga, en la parte inferior
  const int anchoBarra = 128 - posXBarra * 2;  // Ancho de la barra de carga

  // Mapea el porcentaje de carga al ancho de la barra
  int longitudBarra = map(cargaModuloBat, 0, maxCarga, 0, anchoBarra);

  // Dibuja la barra de carga según el porcentaje actual
  u8g2.drawBox(posXBarra, posYBarra, longitudBarra, alturaBarra - 1);
}

void updateu8g2() {
  // Solo actualiza la pantalla si hay datos nuevos
  if (readingsPerSecond > 0) {
    u8g2.clearBuffer();
    // Dibuja los elementos de la interfaz como el ícono del acelerador, el ícono SOC, etc.
    drawThrottleBar(decimalValue);
    drawBatteryIcon(hvSocValue);
    dibujarNivelCarga(cargaModuloBat);

    // Mostrar información variada sobre la energía, temperatura, revoluciones, etc.
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(0, 54);
    u8g2.print("C:");
    u8g2.print(energiaAcumulada, 0);
    u8g2.print(" W");
    u8g2.setCursor(70, 54);
    u8g2.print("R:");
    u8g2.print(energiaRegenerada, 0);
    u8g2.print(" W");
    u8g2.setCursor(45, 64);
    u8g2.print("T:");
    u8g2.print(temperatura, DEC);
    u8g2.setCursor(79, 64);
    u8g2.print("RM:");
    u8g2.print(rpmValor);
    u8g2.setCursor(0, 18);
    u8g2.print("SOC:");
    u8g2.print(hvSocValue, 1);
    u8g2.print("%");
    u8g2.setCursor(0, 33);
    u8g2.print("A:");
    u8g2.print(PAValor, 0);
    drawCurrentIcon(21, PAValor);
    u8g2.setCursor(72, 33);
    u8g2.print("V:");
    u8g2.print(voltajeTotal);
    u8g2.setCursor(38, 33);
    u8g2.print("EV:");
    u8g2.print(EV);
    u8g2.setCursor(109, 33);
    u8g2.print(outputA0Activated ? "Baj" : "Sub");
    u8g2.setCursor(97, 18);
    u8g2.print((hvSocValue <= marcaBaja || hvSocValue >= marcaAlta) ? "N" : "S");

    // Multiplicador y otros valores
    if (outputA0Activated) {
      voltajeModulo = (voltajeV * factor) / 60.0;
      voltajeTotal = (voltajeV * factor);
    } else {
      voltajeModulo = voltajeV / 60.0;
      voltajeTotal = voltajeV;
    }

    u8g2.setCursor(60, 18);

    // Si voltajeV es menor que volLimHiteresis y es momento de mostrar el valor
    if (voltajeV < volLimHiteresis && displayOn) {
      // Convertir voltajeModulo a String con 3 decimales para imprimir
      String voltajeModuloStr = String(voltajeModulo, 3);
      u8g2.print(voltajeModuloStr);
    } else if (voltajeV >= volLimHiteresis) {
      // Siempre muestra el valor si voltajeV es mayor o igual a volLimHiteresis
      // Convertir voltajeModulo a String con 3 decimales para imprimir
      String voltajeModuloStr = String(voltajeModulo, 3);
      u8g2.print(voltajeModuloStr);
    }

    if (PAValor > -mostrarNivelCarga && PAValor < mostrarNivelCarga) {
      ultimoCargaModuloBatValido = cargaModuloBat;  // Guardamos este dulce momento para el futuro
    }
    // Ahora, independientemente de lo que haga PAValor, recordamos y mostramos el último buen momento
    u8g2.setCursor(108, 18);
    u8g2.print(ultimoCargaModuloBatValido);
    u8g2.print("%");
    // Mostrar el valor de mensajes enviados por segundo (E:) en la posición (38, 54) de la pantalla LCD
    u8g2.setCursor(0, 64);
    u8g2.print("E");
    u8g2.print(sentMessagesPerSecond);
    u8g2.print(" L");
    u8g2.print(readingsPerSecond);
    u8g2.sendBuffer();  // Enviar los datos a la pantalla LCD
  }
}


// Declaración de la función para actualizar el estado de la salida A0
void updateOutputA0(bool state) {
  digitalWrite(A0, state ? HIGH : LOW);
}
void processCanMessage(CAN_FRAME& incoming) {
  readCount++;
  readCountPerSecond++;

  // Temperatura

  if (incoming.id == 0x039 && incoming.length == 4) {  // Ajustado el número de bytes a 4
    byte byte0 = incoming.data.byte[0];

    temperatura = byte0;
  }

  // Revoluciones
  if (incoming.id == 0x3C8 && incoming.length == 5) {  // Ajustado el número de bytes a 5
    byte byte2 = incoming.data.byte[2];
    byte byte3 = incoming.data.byte[3];

    rpmValor = ((byte2 * 256) + byte3) / 8;
  }

  // Control condiciones para envío de mensaje SOC
  if (incoming.id == 0x3CB && incoming.length == 7) {
    byte byte2 = incoming.data.byte[2];
    byte byte3 = incoming.data.byte[3];
    hvSocValue = ((byte2 * 256 + byte3) / 2.0);

    // Reactivar mensajeSOC solo si hvSocValue alcanza o supera histeresisAlto
    if (!permitirMensajeSOC && hvSocValue >= histeresisAlto) {
      permitirMensajeSOC = true;
    }

    // Desactivar mensajeSOC una vez que hvSocValue cae por debajo de limiteBajoSOC
    if (hvSocValue < limiteBajoSOC) {
      permitirMensajeSOC = false;
    }

    // Implementación de la histéresis para voltajeV
    if (voltajeV < volLimBajoBat && permitirMensajePorVoltaje) {
      permitirMensajePorVoltaje = false;  // Desactiva debido al voltaje bajo
    } else if (voltajeV >= volLimHiteresis && !permitirMensajePorVoltaje) {
      permitirMensajePorVoltaje = true;  // Reactiva debido al voltaje alto
    }

    // Ejecutar mensajeSOC bajo las condiciones específicas si ambas permitirMensajeSOC y permitirMensajePorVoltaje son true
    if (permitirMensajeSOC && permitirMensajePorVoltaje && hvSocValue >= limiteBajoSOC) {
      // Primera condición específica
      if (hvSocValue < limiteCarga) {  // Control Para que deje de enviar mensajes.
        mensajeSOC();
      }
    }
    if (hvSocValue != lastHvSocValue) {
      lastHvSocValue = hvSocValue;
    }
  }

  // Voltios
  if (incoming.id == 0x03B && incoming.length == 5) {
    byte byte1 = incoming.data.byte[1];
    byte byte3 = incoming.data.byte[3];
    int hvV = (incoming.data.byte[2] * 256) + byte3;
    hvV = (hvV & 0x07FF) - (hvV & 0x0800);
    hvV *= 1;
    // Amperios
    int aValue = ((incoming.data.byte[0]) * 256) + (incoming.data.byte[1]);
    if ((aValue & 0x800) != 0) {
      aValue = aValue - 0x1000;
    }
    PAValor = aValue / 10;

    voltajeV = hvV;
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
    if ((incoming.id == 0x3CB && incoming.length == 7) || (incoming.id == 0x03B && incoming.length == 5) || (incoming.id == 0x529 && incoming.length == 7) || (incoming.id == 0x3CA && incoming.length == 5) || (incoming.id == 0x244 && incoming.length == 8) || (incoming.id == 0x3C9 && incoming.length == 8) || (incoming.id == 0x039 && incoming.length == 4) || (incoming.id == 0x4D1 && incoming.length == 8) || (incoming.id == 0x3C8 && incoming.length == 5)) {

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
    if (canMsg.id == 0x3CA && activacionPendiente == false && canMsg.data.byte[2] <= 0x2D && hvSocValue > 50) {
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
  } else if ((voltajeV <= voltajeMinimo) && (hvSocValue <= socBajo)) {
    // Si lastHvSocValue no cumple la primera condición pero lastByte3 es menor o igual a voltajeMinimo
    if (!lastOutputA0State && elapsedTime >= tiempoInactivoA0) {
      outputA0Activated = false;  // Cambiado a false
      updateOutputA0(outputA0Activated);
      lastOutputA0ChangeMillis = currentMillis;
      lastOutputA0State = true;  // Mantener este true, ya que indica que se realizó un cambio
    }
  } else if (voltajeV >= voltajeMaximo) {
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
void checkConnectionStatus() {
  static unsigned long lastUpdate = millis();  // Mantiene registro de la última actualización
  if (readingsPerSecond > 0) {
    lastUpdate = millis();                    // Restablece el contador si hay datos
  } else if (millis() - lastUpdate > 2000) {  // Más de 2 segundos sin datos
    // Muestra el mensaje de error
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(7, 20, "Sin datos OBD");
    u8g2.drawStr(1, 40, "Vehiculo parado");
    u8g2.drawStr(15, 60, "EN ESPERA");
    u8g2.sendBuffer();
  }
}

void loop() {
  // Lee el estado actual del botón.
  int buttonState = digitalRead(botonPin);

  // Comprueba si el botón se ha pulsado
  if (buttonState == LOW && lastButtonState == HIGH) {
    // Borra el error
    error();
  }

  // Actualiza el estado anterior del botón y el interruptor
  lastButtonState = buttonState;

  /////////////////////////////////////////////////////////////////////

  leerCan();           // Llama a leerCan() en el bucle principal
  activaEv(incoming);  // Pasa el objeto CAN_FRAME como argumento
  conCarga();
  lecSegundo();
  checkConnectionStatus();                                 // Función para verificar el estado de la conexión
  cargaModuloBat = calcularCargaModuloBat(voltajeModulo);  // Calcula el porcentaje de carga de la batería
  unsigned long tiempoActual = millis();

  // Verifica si ha pasado 1 segundo desde la última actualización
  if (tiempoActual - tiempoAnterior >= intervaloActualizacion) {
    // Calcula la potencia instantánea en vatios
    float potenciaInstantanea = voltajeTotal * PAValor;  // PAValor puede ser positivo o negativo

    // Actualiza los acumuladores de energía
    if (PAValor > 0) {
      // Energía consumida
      energiaAcumulada += (potenciaInstantanea / 3600.0);  // Convertir vatios a vatios-hora
    } else if (PAValor < 0) {
      // Energía regenerada
      energiaRegenerada += (-potenciaInstantanea / 3600.0);  // Convertir vatios a vatios-hora, PAValor es negativo
    }

    // Actualiza el tiempo anterior para la próxima iteración
    tiempoAnterior = tiempoActual;
  }
}
