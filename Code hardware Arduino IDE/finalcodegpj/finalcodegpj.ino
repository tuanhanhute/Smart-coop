#include <DHT.h>
#include <FirebaseESP32.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// ================= PIN =================
const int trigPin = 25;
const int echoPin = 26;

#define DHTPIN 5
#define DHTTYPE DHT11

#define FAN_PIN 12
#define HEATER_PIN 14
#define FEEDING_PIN 27
#define PUMP_PIN 32

#define BUTTON1_PIN 16
#define BUTTON2_PIN 17
#define BUTTON3_PIN 18
#define BUTTON4_PIN 19
#define BUTTON5_PIN 33

// ================= THRESHOLD =================
const float pumpStartDistance = 4.0;

// ================= DEVICE =================
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
Servo servo1;

// ================= FIREBASE =================
FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;

// ================= STATE =================
String mode, fan, heater, feeding, pumpState;

int fanState = LOW;
int heaterState = LOW;
int feedingState = LOW;
int pumpStateLocal = LOW;

bool previousPumpState = false;

// ================= DATA STRUCT =================
typedef struct {
  float temp;
  float humi;
} DHTData_t;

typedef struct {
  float distance;
} WaterData_t;

QueueHandle_t dhtQueue;
QueueHandle_t waterQueue;

// ================= LCD UTILS =================
void lcdPrint(const char* l1, const char* l2) {
  lcd.setCursor(2, 0);
  lcd.print("                ");
  lcd.setCursor(2, 0);
  lcd.print(l1);

  lcd.setCursor(2, 1);
  lcd.print("                ");
  lcd.setCursor(2, 1);
  lcd.print(l2);
}

// ================= TASK DHT =================
void Task_DHT(void *pv) {
  DHTData_t data;

  while (1) {
    data.humi = dht.readHumidity();
    data.temp = dht.readTemperature();

    xQueueOverwrite(dhtQueue, &data);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// ================= TASK ULTRASONIC =================
void Task_Ultrasonic(void *pv) {
  WaterData_t w;

  while (1) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    w.distance = duration * 0.034 / 2;

    xQueueOverwrite(waterQueue, &w);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// ================= TASK CONTROL (CORE LOGIC) =================
void Task_Control(void *pv) {
  DHTData_t dhtData;
  WaterData_t waterData;

  while (1) {

    xQueuePeek(dhtQueue, &dhtData, portMAX_DELAY);
    xQueuePeek(waterQueue, &waterData, portMAX_DELAY);

    // ================= READ FIREBASE =================
    Firebase.getString(firebaseData, "/doan1/controller/mode/state");
    mode = firebaseData.stringData();

    Firebase.getString(firebaseData, "/doan1/controller/fan/state");
    fan = firebaseData.stringData();

    Firebase.getString(firebaseData, "/doan1/controller/heater/state");
    heater = firebaseData.stringData();

    Firebase.getString(firebaseData, "/doan1/controller/feeding/state");
    feeding = firebaseData.stringData();

    Firebase.getString(firebaseData, "/doan1/controller/drinking/state");
    pumpState = firebaseData.stringData();

    Firebase.setFloat(firebaseData, "/doan1/controller/temperature", dhtData.temp);
    Firebase.setFloat(firebaseData, "/doan1/controller/humidity", dhtData.humi);

    // ================= MODE SWITCH =================
    if (digitalRead(BUTTON4_PIN) == HIGH) {
      mode = (mode == "auto") ? "user" : "auto";
      Firebase.setString(firebaseData, "/doan1/controller/mode/state", mode);
      vTaskDelay(pdMS_TO_TICKS(300));
    }

    // ================= AUTO MODE =================
    if (mode == "auto") {

      // FAN
      if (dhtData.temp > 28) {
        digitalWrite(FAN_PIN, HIGH);
        fanState = HIGH;
        lcdPrint("Mode: Auto", "FAN: ON");
      } else {
        digitalWrite(FAN_PIN, LOW);
        fanState = LOW;
        lcdPrint("Mode: Auto", "FAN: OFF");
      }

      // HEATER
      if (dhtData.humi > 70) {
        digitalWrite(HEATER_PIN, HIGH);
        heaterState = HIGH;
        lcdPrint("Mode: Auto", "HEATER: ON");
      } else {
        digitalWrite(HEATER_PIN, LOW);
        heaterState = LOW;
        lcdPrint("Mode: Auto", "HEATER: OFF");
      }

      // FEEDING AUTO
      if (feeding == "ON" && feedingState == LOW) {
        servo1.write(0);
        feedingState = HIGH;
        lcdPrint("Mode: Auto", "FEEDING: ON");
      }
      if (feeding == "OFF" && feedingState == HIGH) {
        servo1.write(90);
        feedingState = LOW;
        lcdPrint("Mode: Auto", "FEEDING: OFF");
      }

      // PUMP AUTO
      bool currentPump = waterData.distance > pumpStartDistance;

      if (currentPump != previousPumpState) {
        digitalWrite(PUMP_PIN, currentPump ? HIGH : LOW);
        lcdPrint("Mode: Auto", currentPump ? "PUMP: ON" : "PUMP: OFF");
        previousPumpState = currentPump;
      }
    }

    // ================= USER MODE =================
    else {

      // FAN BUTTON
      if (digitalRead(BUTTON1_PIN) == HIGH) {
        fanState = !fanState;
        digitalWrite(FAN_PIN, fanState);
        Firebase.setString(firebaseData,
          "/doan1/controller/fan/state",
          fanState ? "ON" : "OFF");
        lcdPrint("Mode: User", fanState ? "FAN: ON" : "FAN: OFF");
        vTaskDelay(pdMS_TO_TICKS(300));
      }

      if (fan == "ON") digitalWrite(FAN_PIN, HIGH);
      if (fan == "OFF") digitalWrite(FAN_PIN, LOW);

      // HEATER BUTTON
      if (digitalRead(BUTTON2_PIN) == HIGH) {
        heaterState = !heaterState;
        digitalWrite(HEATER_PIN, heaterState);
        Firebase.setString(firebaseData,
          "/doan1/controller/heater/state",
          heaterState ? "ON" : "OFF");
        lcdPrint("Mode: User", heaterState ? "HEATER: ON" : "HEATER: OFF");
        vTaskDelay(pdMS_TO_TICKS(300));
      }

      if (heater == "ON") digitalWrite(HEATER_PIN, HIGH);
      if (heater == "OFF") digitalWrite(HEATER_PIN, LOW);

      // FEEDING BUTTON
      if (digitalRead(BUTTON3_PIN) == HIGH) {
        feedingState = !feedingState;
        servo1.write(feedingState ? 0 : 90);
        Firebase.setString(firebaseData,
          "/doan1/controller/feeding/state",
          feedingState ? "ON" : "OFF");
        lcdPrint("Mode: User", feedingState ? "FEEDING: ON" : "FEEDING: OFF");
        vTaskDelay(pdMS_TO_TICKS(300));
      }

      // PUMP BUTTON
      if (digitalRead(BUTTON5_PIN) == HIGH) {
        pumpStateLocal = !pumpStateLocal;
        digitalWrite(PUMP_PIN, pumpStateLocal);
        Firebase.setString(firebaseData,
          "/doan1/controller/drinking/state",
          pumpStateLocal ? "ON" : "OFF");
        lcdPrint("Mode: User", pumpStateLocal ? "PUMP: ON" : "PUMP: OFF");
        vTaskDelay(pdMS_TO_TICKS(300));
      }
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  lcd.begin();
  lcd.backlight();

  pinMode(FAN_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);
  pinMode(BUTTON5_PIN, INPUT_PULLUP);

  servo1.attach(FEEDING_PIN);
  dht.begin();

  WiFi.begin("Nha xe C", "88888888");
  while (WiFi.status() != WL_CONNECTED) delay(500);

  config.host = "https://doan1-c4eac-default-rtdb.firebaseio.com/";
  config.signer.tokens.legacy_token = "O5y6r8IZpdEW5HLgVq4BGWNWqUHXMtfBqE35AYW1";
  Firebase.begin(&config, &auth);

  dhtQueue = xQueueCreate(1, sizeof(DHTData_t));
  waterQueue = xQueueCreate(1, sizeof(WaterData_t));

  xTaskCreate(Task_DHT, "DHT", 2048, NULL, 2, NULL);
  xTaskCreate(Task_Ultrasonic, "US", 2048, NULL, 2, NULL);
  xTaskCreate(Task_Control, "CTRL", 4096, NULL, 1, NULL);
}

void loop() {}
