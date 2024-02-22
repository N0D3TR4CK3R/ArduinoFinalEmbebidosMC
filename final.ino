#include <Arduino.h>
#include <DHT.h>
#include "UbidotsEsp32Mqtt.h"

// Definir pines
#define PIN_ENA 19 // L298N enable pin
#define PIN_IN1 18 // L298N IN1 pin
#define PIN_IN2 5  // L298N IN2 pin
#define DHTPIN 13
#define DHTTYPE DHT11
#define LABEL_PUMP "motor" // Ubidots variable label for pump control

DHT dht(DHTPIN, DHTTYPE);

// Sensor de humedad del suelo
#define HUMSOIL_PIN 33
int valHumSoil;

// Ubidots
const char *UBIDOTS_TOKEN = "BBUS-V9aMaBbRJNDaVQuybih9weuei0nHuL";
const char *WIFI_SSID = "CELERITY_CABRERA";
const char *WIFI_PASS = "teEze2022*";
const char *DEVICE_LABEL = "mcabrera-final";
const char *VARIABLE_LABEL_1 = "temperatura";
const char *VARIABLE_LABEL_2 = "humedad";
const char *VARIABLE_LABEL_3 = "humedad_suelo";
const int PUBLISH_FREQUENCY = 5000;
unsigned long timer;

Ubidots ubidots(UBIDOTS_TOKEN);

int pumpState = LOW; // Initial pump state (turned off)

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);

    // Convert received value (assuming it's a string) to integer
    int command = atoi((char*)payload);

    // Validate command (on/off only)
    if (command == 0) {
      pumpState = LOW; // Turn off pump
      Serial.println(" Pump turned off");
    } else if (command == 1) {
      pumpState = HIGH; // Turn on pump
      Serial.println(" Pump turned on");
    } else {
      Serial.println(" Invalid command (use 0/1)");
    }
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  pinMode(HUMSOIL_PIN, INPUT);

 // Set L298N pins as outputs
  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);

  // Turn off pump initially
  digitalWrite(PIN_ENA, LOW);
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);

  ledcSetup(0, 5000, 8);

  dht.begin();

  // Ubidots
  ubidots.setDebug(true);
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected to WiFi");
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();
  timer = millis();
  ubidots.subscribeLastValue(DEVICE_LABEL, LABEL_PUMP);
}

void loop() {

  if (!ubidots.connected()) {
    ubidots.reconnect();
  }

  if (labs(static_cast<long>(millis()) - static_cast<long>(timer)) > PUBLISH_FREQUENCY) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    valHumSoil = map(analogRead(HUMSOIL_PIN), 0, 4095, 0, 100);

    ubidots.add(VARIABLE_LABEL_1, t);
    ubidots.add(VARIABLE_LABEL_2, h);
    ubidots.add(VARIABLE_LABEL_3, valHumSoil);
    ubidots.publish(DEVICE_LABEL);

    Serial.println("Enviando datos a Ubidots:");
    Serial.println("Temperatura: " + String(t));
    Serial.println("Humedad: " + String(h));
    Serial.println("Humedad del suelo: " + String(valHumSoil));
    Serial.println("-----------------------------------------");

    timer = millis();
  }

  ubidots.loop();

  // Set L298N pins based on pump state
  digitalWrite(PIN_ENA, pumpState);
  if (pumpState == HIGH) {
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
  } else {
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);
  }
}
