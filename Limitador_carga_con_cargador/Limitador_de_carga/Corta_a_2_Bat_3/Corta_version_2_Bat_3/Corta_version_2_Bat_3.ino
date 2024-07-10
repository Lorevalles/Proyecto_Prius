#include <Wire.h>
#include <ADS1115.h>
ADS1115Scanner adc;
ADS1115ScaleFloat scale0;

const int BAUD_RATE = 9600;
const uint32_t AVERAGE_SAMPLES = 100;
const int ADC_RANGE = ADS1115_RANGE_4096;
const int REF_VOLTAGE = -30082; // Valor entrada
const int SCALE_FACTOR = 798;    // Voltios en salida x10 para int
const int OUTPUT_PIN = 13;

void setup() {
  Serial.begin(BAUD_RATE);
  pinMode(OUTPUT_PIN, OUTPUT);
  Wire.begin();
  adc.setSpeed(ADS1115_SPEED_8SPS);
  scale0.setRef(0, 0, REF_VOLTAGE, SCALE_FACTOR);
  adc.addChannel(ADS1115_CHANNEL01, ADC_RANGE);
  adc.setSamples(AVERAGE_SAMPLES);
  adc.start();
  Serial.println("Un momento, calculando...");
}

uint32_t iterationCount = 0;
void loop() {
  iterationCount++;
  adc.update();
  if (adc.ready()) {
    float averageValue = adc.readAverage(0);
    float scaledValue = (scale0.scale(averageValue)) / 10;
    Serial.print("Valor diferencial en la entrada #0 #1 es: ");
    Serial.println(averageValue);
    Serial.print(scaledValue, 2);
    Serial.println(" Voltios");
    Serial.println("Calculados como promedio de 100 mediciones.");
    Serial.print("Mientras hacía esto, el Arduino contó hasta: ");
    Serial.println(iterationCount);
    Serial.println();
    iterationCount = 0;
    adc.start();

    digitalWrite(OUTPUT_PIN, LOW);

    // Si el valor es mayor que 81V, activa la salida digital
    if (scaledValue > 81) {
      digitalWrite(OUTPUT_PIN, HIGH);
    }
  }
}
