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
#define WIFI_SSID "IoT_Network" // Local network (change to own)
#define WIFI_PASSWORD "p455w0rd" // Local network password (change to own)

// MQTT
#define MQTT_HOST "iotmadnessproject.hopto.org" // Server domain name
#define MQTT_PORT 1883

#define MQTT_USERNAME "iot"
#define MQTT_PASSWORD "iot"

#define MQTT_STATS_TOPIC_PUB "PlantStats"
#define MQTT_WATER_TOPIC_SUB "WaterBroadcast"

// Ultrasonic sensor
#define TRIGGER_PIN 22
#define ECHO_PIN 23
#define MAX_DISTANCE 500
#define US_SENSOR_SAMPLES 30
#define WL_MAXIMUM 3
#define WL_WARNING 20
#define WL_MINIMUM 22
#define WL_EMPTY 24

// Humidity sensor
#define WATERING_STATE 200
#define WET_STATE 110
#define DRY_STATE 70
#define SH_SENSOR_SAMPLES 20
#define HUMIDITY_SENSOR_PIN 34

// Water pump
#define WATER_PUMP_PIN 32 // Relay pin

// General
#define DEBUG true
#define BAUD_RATE 115200
#define MANUAL_CONTROL 2
#define MANUAL_REPORT 4
#define FAIL_STATE 18
#define INIT_STATE 21
#define REPORT_INTERVAL 1800000 // 1800000 millis = 30 minutes
#define WATER_INTERVAL 259200000 // 259200000 millis = 3 days

unsigned long previousMillis = 0;
bool out = true, init_success = false, water_flag = false;
vector<String> id_buf;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

void status_report();

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
  bool target_found = true;

  // Check if the MQTT message was received on WaterBroadcast
  if (strcmp(topic, MQTT_WATER_TOPIC_SUB) == 0) {
    if (DEBUG) Serial.println("Correct WaterBroadcast topic");

    if (len == id_buf[0].length()-1) { 
      for (int i = 0; i < len; i++) {
        messageTemp += (char)payload[i];
        if (payload[i] != id_buf[0][i]) {
          target_found = false;
          break;
        }
      }
    } else target_found = false;

    // Compare system id
    if (target_found) {
      if (DEBUG) Serial.println("TARGET SYSTEM FOUND. WATERING...");
      water_flag = true;
    } else {
      if (DEBUG) Serial.println("TARGET SYSTEM NOT FOUND");
    }
  }

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

void waterPump(int duration = 4500) {
  // Pump water for [duration/1000] seconds
  digitalWrite(WATER_PUMP_PIN, 0);
  delay(duration);
  digitalWrite(WATER_PUMP_PIN, 1);

  // Report new plant status
  status_report();
}

unsigned long soilHumidityCapture() {
  unsigned long soil_capture_avg = 0.0;

  for (int i = 0; i < SH_SENSOR_SAMPLES; i++) {
    soil_capture_avg += analogRead(HUMIDITY_SENSOR_PIN);
    delay(75);
  }

  soil_capture_avg /= (float)SH_SENSOR_SAMPLES;
  if (DEBUG) Serial.println("Soil humidity sensor avg: " + String(soil_capture_avg));

  return soil_capture_avg;
}

unsigned long waterLevelCapture() {
  // Take water level via ultrasonic sensor
  unsigned long capture_avg = 0;

  for (int i = 0; i < US_SENSOR_SAMPLES; i++) {
    capture_avg += sonar.ping_cm();
    delay(75);
  }
  
  capture_avg /= (float)US_SENSOR_SAMPLES;
  if (DEBUG) Serial.println("Sonar sensor avg: " + String(capture_avg) + "cm");

  return capture_avg;
}

void initSystem() {
  if (DEBUG) Serial.println("Initializing system...");

  unsigned long starting_time = millis(), millisHelper = millis();
  while (millisHelper - starting_time < 66000) {
    soilHumidityCapture();
    millisHelper = millis();
    if (DEBUG) Serial.println("Progress: " + String(map(millisHelper - starting_time, starting_time, 66000, 0, 100)) + "%");
  }
  
  if (DEBUG) Serial.println("Initialization complete");
}

void status_report() {
  String soil_stat, water_stat;
  unsigned long soil_capture = soilHumidityCapture(), water_capture = waterLevelCapture();

  if (soil_capture > WATERING_STATE + 100) soil_capture = WATERING_STATE + 100;

  soil_capture = map(soil_capture, DRY_STATE, WATERING_STATE + 100, 0, 100);
  water_capture = map(water_capture, WL_EMPTY, WL_MAXIMUM, 0, 100);

  if (soil_capture >= WATERING_STATE) {
    soil_stat = "Dry";
  } else if (soil_capture < WATERING_STATE && soil_capture >= WET_STATE) {
    soil_stat = "Wet";
  } else if (soil_capture < WET_STATE && soil_capture >= DRY_STATE) {
    soil_stat = "Watering";
  }

  if (water_capture >= WL_EMPTY) {
    water_stat = "Maximum";
  } else if (water_capture < WL_EMPTY && water_capture >= WL_MINIMUM) {
    water_stat = "Warning";
  } else if (water_capture < WL_MINIMUM && water_capture >= WL_WARNING) {
    water_stat = "Minimum";
  } else if (water_capture < WL_WARNING && water_capture >= WL_MAXIMUM) {
    water_stat = "Empty";
  }

  String report_msg = "HUMIDITY:" + String(soil_capture) + "%(" + soil_stat + ") WATER_LEVEL:" + String(water_capture) + "%(" + water_stat +")" + " " + id_buf[0];
  uint16_t status_holder = mqttClient.publish(MQTT_STATS_TOPIC_PUB, 1, true, String(report_msg).c_str());
  if (DEBUG) Serial.println(report_msg);
  if (DEBUG) Serial.printf("Publishing on topic %s at QoS 1, packetId: %i; Message: ", MQTT_STATS_TOPIC_PUB, status_holder);
}

void setup() {
  Serial.begin(BAUD_RATE);

  pinMode(HUMIDITY_SENSOR_PIN, INPUT);
  pinMode(MANUAL_CONTROL, INPUT);
  pinMode(MANUAL_REPORT, INPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  pinMode(INIT_STATE, OUTPUT);
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
      if (DEBUG) Serial.println(s);
    }
  } catch (...) {
    if (DEBUG) Serial.println("Error while reading id file.");
    digitalWrite(FAIL_STATE, 1);
    return;
  }

  initSystem();
  init_success = true;
  digitalWrite(INIT_STATE, 1);

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

  // // MQTT Connection
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USERNAME, MQTT_PASSWORD);
  connectToWifi();
}

void loop() {
  unsigned long currentMillis = millis();

  if (water_flag) {
    waterPump(3500);
    water_flag = false;
  }

  if (init_success) {
    // Report system status every 30 minutes
    if (currentMillis - previousMillis >= REPORT_INTERVAL) {
      status_report();
    }

    // Water plant every 3 days
    if (currentMillis - previousMillis >= WATER_INTERVAL) {
      waterPump(5500);
    }

    previousMillis = currentMillis;
  }

  if (digitalRead(MANUAL_CONTROL) == 1) {
    digitalWrite(WATER_PUMP_PIN, 0);
  } else if (digitalRead(MANUAL_CONTROL) == 0) {
    digitalWrite(WATER_PUMP_PIN, 1);
  }

  if (digitalRead(MANUAL_REPORT) == 1) {
    status_report();
    delay(250);
  }
}
