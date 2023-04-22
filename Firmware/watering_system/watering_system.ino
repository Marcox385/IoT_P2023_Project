/*
 * Watering System - IoT Project - Spring 2023
 *
 * Members:
 *  - IS727272 Cordero Hernández, Marco Ricardo
 *  - SI727576 Guzmán Claustro, Edgar
 *  - IS727550 Díaz Aguayo, Adriana
 *  - IS727366 Rodríguez Castro, Carlos Eduardo 
*/

int potValue = 0;
int potpot = 0;

void setup() {
  Serial.begin(115200);
  pinMode(2, INPUT);
  // pinMode(15, INPUT);
}

void loop() {
  delay(750); // Espera de 3/4 de segundo)

  potpot = analogRead(2);
  potValue = map(potpot, 0, 1023, 255, 0);
  Serial.print("Analog: ");
  Serial.println(potpot);
  Serial.println(potValue);

  // potValue = analogRead(15); // Lectura de  valor análogo
  // Serial.print("Analog: ");
  // Serial.println(potValue);
}
