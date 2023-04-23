/*
 * Watering System - IoT Project - Spring 2023
 *
 * Members:
 *  - IS727272 Cordero Hernández, Marco Ricardo
 *  - SI727576 Guzmán Claustro, Edgar
 *  - IS727550 Díaz Aguayo, Adriana
 *  - IS727366 Rodríguez Castro, Carlos Eduardo 
*/

// Libraries section
// #include <WiFi.h>
// extern "C" {
// #include "freertos/FreeRTOS.h"
// #include "freertos/timers.h"
// }
// #include <AsyncMqttClient.h>
#include <NewPing.h>

// /* Constant definition */
// // Wi-Fi
// #define WIFI_SSID "IoT_Network" // Local network (change to own)
// #define WIFI_PASSWORD "IoT_P455W0RD" // Local network password (change to own)

// Ultrasonic sensor
#define TRIGGER_PIN 22
#define ECHO_PIN 23
#define MAX_DISTANCE 500

// Humidity sensor
#define WET_STATE 1600
#define IDEAL_STATE 2300
#define DRY_STATE 3000
#define HUMIDITY_SENSOR_PIN 15

// Water pump
#define WATER_PUMP_PIN 32 // Relay pin

// General
#define BAUD_RATE 115200

bool out = true;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

void setup() {
  Serial.begin(BAUD_RATE);
  pinMode(HUMIDITY_SENSOR_PIN, INPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
}

void waterPump() {

}

void soilHumidityCapture() {
  int humidity_value = analogRead(HUMIDITY_SENSOR_PIN);

  /*
    TODO:
      - Process read value to send MQTT message
      - Add intervals (<=1600 soaking wet, 2300 ideal value, >=3000 dry)
  */
}

void waterLevelCapture() {
  // Take water level via ultrasonic sensor
  // TODO: Implement predefined water container values

  unsigned long capture_avg = 0;

  for (int i = 0; i < 20; i++) {
    capture_avg += sonar.ping_cm();
    delay(75);
  }
  
  capture_avg /= 20.0;
  Serial.println("Sonar sensor avg: " + String(capture_avg) + "cm");
}

void loop() {
  // out = !out;
  // digitalWrite(HUMIDITY_SENSOR_PIN, (out) ? 1 : 0);

  // Serial.print("Value: ");
  // Serial.println((out) ? 1 : 0);

  delay(2000);
  // Serial.println("Sensor: " + String(analogRead(HUMIDITY_SENSOR_PIN)));
  waterLevelCapture();
}
