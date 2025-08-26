#include <WiFi.h>
#include "DHTesp.h"
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

// Hardware pin definitions
#define DHT_PIN     15
#define LDR_PIN     34  // Light dependent resistor on ADC1_CH6
#define SERVO_PIN   14

// System timing variables
unsigned long samplingInterval = 1;     
unsigned long sendingInterval  = 10;   
unsigned long lastSampleTime   = 0;  
unsigned long lastSendTime     = 0;  

// Light measurement variables
uint32_t sumLight   = 0;  
uint16_t sampleCount = 0;  
float intensity;

// Servo control parameters
float thetaOffset  = 30;
float gammaFactor  = 0.75;
float Tmed         = 30;

// Temperature storage
char tempAr[6];

// Scheduling variables
bool isScheduledON = false;
unsigned long scheduledOnTime;

// Network objects
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Sensor objects
DHTesp dhtSensor;
Servo windowServo;

// Function declarations
void setupWifi();
void setupMqtt();
void connectToBroker();
void updateTemperature();
unsigned long getTime();
void receiveCallbackMyFunction(char* topic, byte* payload, unsigned int length);

/**
 * Initial setup - runs once at start
 */
void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Configure hardware pins
  pinMode(LDR_PIN, INPUT);

  // Initialize network connections
  setupWifi();
  setupMqtt();
  
  // Configure time client
  timeClient.begin();
  timeClient.setTimeOffset(19800);  // GMT+5:30
  
  // Initialize sensors and actuators
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  windowServo.attach(SERVO_PIN);
  windowServo.write(thetaOffset);
}

/**
 * Main program loop - runs continuously
 */
void loop() {
  // Ensure MQTT connection
  if(!mqttClient.connected()) {
    connectToBroker();
  }
  mqttClient.loop();

  // Update and publish temperature
  updateTemperature();
  mqttClient.publish("BOX-TEMP", tempAr);

  unsigned long now = millis();

  // Sample light at regular intervals
  if (now - lastSampleTime >= samplingInterval) {
    lastSampleTime = now;
    uint16_t raw = analogRead(LDR_PIN);
    sumLight += raw;                         
    sampleCount++;
  }

  // Process and publish light data at regular intervals
  if (now - lastSendTime >= sendingInterval) {
    lastSendTime = now;
    
    // Calculate average light intensity
    float avg = (sampleCount > 0) ? float(sumLight)/sampleCount/4095.0f : 0.0f;
    intensity = 1.0f - avg;
    
    // Publish light reading
    char buf[8];
    dtostrf(intensity, 1, 3, buf);
    mqttClient.publish("BOX-LIGHT", buf);

    // Reset accumulators
    sumLight = 0;
    sampleCount = 0;
    
    // Calculate servo position based on environmental factors
    float T = dhtSensor.getTempAndHumidity().temperature;
    float ratio = float(samplingInterval) / float(sendingInterval);
    if (ratio <= 0) ratio = 1.0f;

    // Calculate window angle
    float theta = thetaOffset + 
                 (180.0f - thetaOffset) * 
                  intensity * 
                  gammaFactor * 
                  log(ratio) * 
                  (T / Tmed);

    // Constrain angle to valid range
    theta = constrain(theta, 0.0f, 180.0f);
    
    // Set servo position
    windowServo.write(theta);

    // Log diagnostic information
    Serial.println("----------------");
    Serial.print("New θ = ");      Serial.println(theta);
    Serial.print("New gmma = ");   Serial.println(gammaFactor);
    Serial.print("New Tmed = ");   Serial.println(Tmed);
    Serial.print("New offset = ");  Serial.println(thetaOffset);
    Serial.print("Intensity = ");   Serial.println(intensity);
    Serial.print("ratio = ");       Serial.println(ratio);
    Serial.print("T = ");           Serial.println(T);
    Serial.println("----------------");
  }
  
}

/**
 * Updates current time from NTP server
 */
unsigned long getTime() {
  timeClient.update();
  return timeClient.getEpochTime();
}


/**
 * Configures MQTT client and callback
 */
void setupMqtt() {
  mqttClient.setServer("broker.hivemq.com", 1883);
  mqttClient.setCallback(receiveCallbackMyFunction);
}

/**
 * Processes incoming MQTT messages
 */
void receiveCallbackMyFunction(char* topic, byte* payload, unsigned int length) {
  // Print debug information
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  // Convert payload to char array
  char payloadCharAr[length + 1];
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }
  payloadCharAr[length] = '\0';
  Serial.println();

  // Process message based on topic
  
  if (strcmp(topic, "ENTC-ADMIN-SCH-ON") == 0) {
    // Set schedule
    if (payloadCharAr[0] == 'N') {
      isScheduledON = false;
    } else {
      isScheduledON = true;
      scheduledOnTime = atol(payloadCharAr);
    }
  }
  else if (strcmp(topic, "light_sampling_int") == 0) {
    // Update sampling interval (seconds → ms)
    samplingInterval = atol(payloadCharAr) * 1000UL;
    Serial.println(samplingInterval);
  }
  else if (strcmp(topic, "light_value_int") == 0) {
    // Update sending interval (seconds → ms)
    sendingInterval = atol(payloadCharAr) * 1000UL;
    Serial.println(sendingInterval);
  }
  else if (strcmp(topic, "min_angle") == 0) {
    // Update minimum servo angle
    thetaOffset = constrain(atof(payloadCharAr), 0.0f, 120.0f);
  }
  else if (strcmp(topic, "control_factor") == 0) {
    // Update gamma factor
    gammaFactor = constrain(atof(payloadCharAr), 0.0f, 1.0f);      
  }
  else if (strcmp(topic, "sto_temp") == 0) {
    // Update reference temperature
    Tmed = constrain(atof(payloadCharAr), 10.0f, 40.0f);      
  }
}

/**
 * Establishes connection to MQTT broker
 */
void connectToBroker() {
  while (!mqttClient.connected()) {
    Serial.print("MQTT connection...");
    
    if (mqttClient.connect("ESP32-12345645498")) {
      Serial.println("Successfull");
      
      // Subscribe to control topics
      mqttClient.subscribe("ENTC-ADMIN-MAIN-ON-OFF");
      mqttClient.subscribe("ENTC-ADMIN-SCH-ON");
      mqttClient.subscribe("light_sampling_int");
      mqttClient.subscribe("light_value_int");
      mqttClient.subscribe("min_angle");
      mqttClient.subscribe("control_factor");
      mqttClient.subscribe("sto_temp");
    } else {
      Serial.print("failed rc=");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

/**
 * Reads temperature data from DHT sensor
 */
void updateTemperature() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature, 2).toCharArray(tempAr, 6);
}

/**
 * Establishes WiFi connection
 */
void setupWifi() {
  const char* ssid = "Wokwi-GUEST";
  const char* password = "";
  
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}