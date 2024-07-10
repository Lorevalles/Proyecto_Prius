//Lectura y escritura Arduino Due y SN65HVD230 CAN OBD2

#include <due_can.h>

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

uint32_t receivedFrames;
CAN_FRAME incoming;

void error() { // Borra los códigos de error del vehiculo (Toyota Prius)
  // Mensaje CAN ID 7DF (11 bits), DLC 8, Datos 02 01 00 00 00 00 00 00
  uint8_t data1[] = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  sendCANMessage(0x7DF, 8, data1);

  // Mensaje CAN ID 7E0 (11 bits), DLC 8, Datos 02 3E 00 00 00 00 00 00
  uint8_t data2[] = {0x02, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  sendCANMessage(0x7E0, 8, data2);

  // Mensaje CAN ID 7E2 (11 bits), DLC 8, Datos 02 3E 00 00 00 00 00 00
  uint8_t data3[] = {0x02, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  sendCANMessage(0x7E2, 8, data3);

  // Mensaje CAN ID 7E3 (11 bits), DLC 8, Datos 02 3E 00 00 00 00 00 00
  uint8_t data4[] = {0x02, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  sendCANMessage(0x7E3, 8, data4);
}

void setup() {
  Serial.begin(115200);

  if (Can0.begin(CAN_BPS_500K)) {
    Serial.println("CAN0 initialization successful");
  } else {
    Serial.println("CAN0 initialization error");
  }
  error(); // llamada para borrar errores

  // Configura los filtros para los ID de interés
  Can0.watchFor(0x3CB);       // ID 0x3CB, DLC 7, SOC Batería
  Can0.watchFor(0x03B);       // ID 0x03B, DLC 5, Voltaje batería EV
  Can0.watchFor(0x529);       // ID 0x529, DLC 7, Estado EV (botón)
  Can0.watchFor(0x3CA);       // ID 0x3CA, DLC 5, Velocidad
  Can0.watchFor(0x244);       // ID 0x244, DLC 8, Acelerarador
  Can0.watchFor(0x07E221C4);  // ID 0x07E221C4, DLC 8, Ángulo acelerador
  Can0.watchFor(0x18DB33F1);  // ID 0x18DB33F1, DLC 8, Pregunta conexion 7DF equivalente
}

void loop() {
  // Variables locales para almacenar mensajes CAN recibidos
  CAN_FRAME incoming;

  // Comprueba si hay datos CAN recibidos
  if (Can0.available()) {
    Can0.read(incoming);

    // Verifica si el mensaje cumple con alguna de las condiciones
    if ((incoming.id == 0x3CB && incoming.length == 7) || (incoming.id == 0x03B && incoming.length == 5) || (incoming.id == 0x529 && incoming.length == 7) || (incoming.id == 0x3CA && incoming.length == 5) || (incoming.id == 0x244 && incoming.length == 8) || (incoming.id == 0x07E221C4 && incoming.length == 8) || (incoming.id == 0x18DB33F1 && incoming.length == 8)) {
      // Imprime el ID y los datos recibidos
      Serial.print("ID: 0x");
      Serial.print(incoming.id, HEX);
      Serial.print(", Datos: ");
      for (byte i = 0; i < incoming.length; i++) {
        Serial.print("0x");
        Serial.print(incoming.data.byte[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      receivedFrames++;
    }
  }
}
