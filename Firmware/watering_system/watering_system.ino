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
#include <WiFi.h>
extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}
#include <AsyncMqttClient.h>
#include <NewPing.h>
#include <SPIFFS.h>
using namespace std;

/* Constant definition */
// Wi-Fi
#define WIFI_SSID "Totalplay-1DAA" // Local network (change to own)
#define WIFI_PASSWORD "1DAA8000CeEqD6dX" // Local network password (change to own)

// MQTT
#define MQTT_HOST "iotmadnessproject.hopto.org" // Server domain name
#define MQTT_PORT 1883

#define MQTT_USERNAME_PUB "iot"
#define MQTT_PASSWORD_PUB "iot"

#define MQTT_STATS_TOPIC_PUB "PlantStats"
#define MQTT_WATER_TOPIC_SUB "WaterBroadcast"

// Ultrasonic sensor
#define TRIGGER_PIN 22
#define ECHO_PIN 23
#define MAX_DISTANCE 500
#define US_SENSOR_SAMPLES 30

// Humidity sensor
#define WET_STATE 1600
#define IDEAL_STATE 2300
#define DRY_STATE 3000
#define SH_SENSOR_SAMPLES 50
#define HUMIDITY_SENSOR_PIN 15

// Water pump
#define WATER_PUMP_PIN 32 // Relay pin

// General
#define DEBUG true
#define BAUD_RATE 115200
#define MANUAL_CONTROL 2
#define FAIL_STATE 18
#define INTERVAL_FACTOR 1800000 // 1800000 millis = 30 minutes

bool out = true, init_success = false;
vector<String> id_buf;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

void connectToWifi() {
  if (DEBUG) Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void WiFiEvent(WiFiEvent_t event) {
  if (DEBUG) Serial.printf("[WiFi-event] event: %d\n", event);
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      if (DEBUG) {
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
      }

      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      if (DEBUG) Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0);  // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}

void connectToMqtt() {
  if (DEBUG) Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  if (DEBUG) {
    Serial.println("Connected to MQTT.");
    Serial.print("Session present: ");
    Serial.println(sessionPresent);
  }

  uint16_t packetIdSub = mqttClient.subscribe(MQTT_WATER_TOPIC_SUB, 0);

  if (DEBUG) Serial.println("Subscribing at QoS 0");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  if (DEBUG) Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttPublish(uint16_t packetId) {
  if (DEBUG) {
    Serial.print("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  if (DEBUG) {
    Serial.println("Subscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
    Serial.print("  qos: ");
    Serial.println(qos);
  }
}

void onMqttUnsubscribe(uint16_t packetId) {
  if (DEBUG) {
    Serial.println("Unsubscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  String messageTemp;
  for (int i = 0; i < len; i++) {
    messageTemp += (char)payload[i];
  }

  // Check if the MQTT message was received on topic test
  if (strcmp(topic, MQTT_WATER_TOPIC_SUB) == 0) {
    Serial.println("TRUE");
  }

  // if (messageTemp == "TRUE") digitalWrite(LED, HIGH);
  // else if (messageTemp == "FALSE") digitalWrite(LED, LOW);

  if (DEBUG) {
    Serial.println("Publish received.");
    Serial.print("  message: ");
    Serial.println(messageTemp);
    Serial.print("  topic: ");
    Serial.println(topic);
    Serial.print("  qos: ");
    Serial.println(properties.qos);
    Serial.print("  dup: ");
    Serial.println(properties.dup);
    Serial.print("  retain: ");
    Serial.println(properties.retain);
    Serial.print("  len: ");
    Serial.println(len);
    Serial.print("  index: ");
    Serial.println(index);
    Serial.print("  total: ");
    Serial.println(total);
  }
}

void setup() {
  Serial.begin(BAUD_RATE);

  pinMode(HUMIDITY_SENSOR_PIN, INPUT);
  pinMode(MANUAL_CONTROL, INPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  pinMode(FAIL_STATE, OUTPUT);

  digitalWrite(WATER_PUMP_PIN, 1); // Prevent relay activation at power-on

  if(!SPIFFS.begin(true)){
    if (DEBUG) Serial.println("Error occurred while mounting SPIFFS.\nUnable to read system ID.");
    digitalWrite(FAIL_STATE, 1);
    return;
  }

  File id_file = SPIFFS.open("/id_file.txt");
  
  if(!id_file){
    if (DEBUG) Serial.println("Failed to open id file for reading.");
    digitalWrite(FAIL_STATE, 1);
    return;
  }

  try {
    while (id_file.available()) {
      id_buf.push_back(id_file.readStringUntil('\n'));
    }
    id_file.close();

    for (String s : id_buf) {
      Serial.println(s);
    }
  } catch (...) {
    if (DEBUG) Serial.println("Error while reading id file.");
    digitalWrite(FAIL_STATE, 1);
    return;
  }
  
  init_success = true;

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  // MQTT Setup
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);

  // MQTT Publish
  mqttClient.onPublish(onMqttPublish);

  // MQTT Subscribe
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);

  // MQTT Connection
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USERNAME_SUB, MQTT_PASSWORD_SUB);
  connectToWifi();
}

void waterPump() { }

unsigned long soilHumidityCapture() {
  unsigned long capture_avg = 0;

  for (int i = 0; i < 20; i++) {
    capture_avg += analogRead(HUMIDITY_SENSOR_PIN);
    delay(75);
  }

  capture_avg /= (float)SH_SENSOR_SAMPLES;
  Serial.println("Soil humidity sensor avg: " + String(capture_avg));

  /*
    TODO:
      - Process read value to send MQTT message
      - Add intervals (<=1600 soaking wet, 2300 ideal value, >=3000 dry)
  */
}

unsigned long waterLevelCapture() {
  // Take water level via ultrasonic sensor
  // TODO: Implement predefined water container values {24: empty, 22: minimum, 20: warning, 3: maximum}
  // Values defined for test water container

  unsigned long capture_avg = 0;

  for (int i = 0; i < US_SENSOR_SAMPLES; i++) {
    capture_avg += sonar.ping_cm();
    delay(75);
  }
  
  capture_avg /= (float)US_SENSOR_SAMPLES;
  if (DEBUG) Serial.println("Sonar sensor avg: " + String(capture_avg) + "cm");

  return capture_avg;
}

void test_publish(char* msg) {
  uint16_t packetIdPub1 = mqttClient.publish(MQTT_STATS_TOPIC_PUB, 1, true, String(msg).c_str());
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i; Message: ", MQTT_STATS_TOPIC_PUB, packetIdPub1);
}

void loop() {
  if (init_success) {
    // out = !out;
    // digitalWrite(HUMIDITY_SENSOR_PIN, (out) ? 1 : 0);
  
    // Serial.print("Value: ");
    // Serial.println((out) ? 1 : 0);
  
    // Serial.println("Sensor: " + String(analogRead(HUMIDITY_SENSOR_PIN)));
    if (digitalRead(MANUAL_CONTROL) == 1) {
      // waterLevelCapture();
      soilHumidityCapture();
    }
    digitalWrite(WATER_PUMP_PIN, (digitalRead(MANUAL_CONTROL) == 1) ? 0 : 1);
  
    delay(500);

    // if (digitalRead(MANUAL_CONTROL) == 1) Serial.println(id_buf[0]);
    // if (digitalRead(MANUAL_CONTROL) == 1) Serial.println(analogRead(HUMIDITY_SENSOR_PIN));
  }
}
