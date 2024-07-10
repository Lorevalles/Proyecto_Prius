#include <can.h>
#include <mcp2515.h>
#include <CanHacker.h>
#include <CanHackerLineReader.h>
#include <lib.h>

#include <SPI.h>
#include <SoftwareSerial.h>

const int SPI_CS_PIN = 10;
const int INT_PIN = 2;  // 2
const int SS_RX_PIN = 11;
const int SS_TX_PIN = 12;

CanHackerLineReader *lineReader = NULL;               // Puntero a objeto CanHackerLineReader
CanHacker *canHacker = NULL;                          // Puntero a objeto CanHacker
SoftwareSerial softwareSerial(SS_RX_PIN, SS_TX_PIN);  // Objeto SoftwareSerial para la comunicación serie

void setup() {
  Serial.begin(115200);  // Inicia la comunicación serie por el puerto serial
  while (!Serial)
    ;                            // Espera hasta que la comunicación serie esté establecida
  SPI.begin();                   // Inicializa la comunicación SPI
  softwareSerial.begin(115200);  // Inicia la comunicación serie por el objeto SoftwareSerial

  Stream *interfaceStream = &Serial;      // Puntero a la interfaz de transmisión (puerto serial)
  Stream *debugStream = &softwareSerial;  // Puntero a la interfaz de depuración (SoftwareSerial)

  canHacker = new CanHacker(interfaceStream, debugStream, SPI_CS_PIN);  // Crea un nuevo objeto CanHacker
  //canHacker->enableLoopback(); // Descomentar para habilitar el modo de loopback
  lineReader = new CanHackerLineReader(canHacker);  // Crea un nuevo objeto CanHackerLineReader utilizando el objeto CanHacker

  pinMode(INT_PIN, INPUT);  // Configura el pin de interrupción como entrada
}

void loop() {
  CanHacker::ERROR error;

  if (digitalRead(INT_PIN) == LOW) {        // Verifica si el pin de interrupción está en estado bajo
    error = canHacker->processInterrupt();  // Procesa la interrupción del MCP2515
    handleError(error);                     // Maneja cualquier error devuelto
  }

  error = lineReader->process();  // Procesa una línea de entrada
  handleError(error);             // Maneja cualquier error devuelto
}

void handleError(const CanHacker::ERROR error) {
  switch (error) {
    case CanHacker::ERROR_OK:
    case CanHacker::ERROR_UNKNOWN_COMMAND:
    case CanHacker::ERROR_NOT_CONNECTED:
    case CanHacker::ERROR_MCP2515_ERRIF:
    case CanHacker::ERROR_INVALID_COMMAND:
      return;  // Sale de la función si el error es uno de los casos enumerados

    default:
      break;  // Continúa con el manejo de errores por defecto
  }

  softwareSerial.print("Failure (code ");  // Imprime un mensaje de fallo
  softwareSerial.print((int)error);        // Imprime el código de error
  softwareSerial.println(")");

  //digitalWrite(SPI_CS_PIN, HIGH);  // Establece el pin de chip select (CS) del MCP2515 en alto
  pinMode(LED_BUILTIN, OUTPUT);  // Configura el pin LED_BUILTIN como salida

  while (1) {  // Bucle infinito para parpadear el LED en función del código de error
    int c = (int)error;
    for (int i = 0; i < c; i++) {
      digitalWrite(LED_BUILTIN, HIGH);  // Enciende el LED
      delay(500);                       // Espera 500 ms
      digitalWrite(LED_BUILTIN, LOW);   // Apaga el LED
      delay(500);                       // Espera 500 ms
    }

    delay(2000);  // Pausa de 2 segundos antes de comenzar la siguiente secuencia de parpadeo
  }
}
