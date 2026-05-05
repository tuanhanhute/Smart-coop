#include <DHT.h>
#include <FirebaseESP32.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

const int trigPin = 25;
const int echoPin = 26;
const float pumpStartDistance = 4.0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

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

#define WIFI_SSID "Nha xe C"
#define WIFI_PASSWORD "88888888"

FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;

DHT dht(DHTPIN, DHTTYPE);
Servo servo1;

String mode, heater, fan, State, StatePump;

int fanState = LOW;
int heaterState = LOW;
int StateFeeding = LOW;
int StatePumping = LOW;
bool previousPumpState = false;

typedef struct {
  float temp;
  float humi;
} DHTData_t;

QueueHandle_t dhtQueue;

// ================= PUMP FUNCTION =================
void checkWaterLevelAndPump() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distanceToWater = duration * 0.034 / 2;
  bool currentPumpState = distanceToWater > pumpStartDistance;

  if (currentPumpState != previousPumpState) {
    digitalWrite(PUMP_PIN, currentPumpState ? HIGH : LOW);
    previousPumpState = currentPumpState;
  }
}

// ================= READ DHT =================
void Task_DHT(void *pv) {
  DHTData_t data;
  for (;;) {
    data.humi = dht.readHumidity();
    data.temp = dht.readTemperature();
    xQueueSend(dhtQueue, &data, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// ================= TASK PUMP WATER =================
void Task_Pump(void *pv) {
  for (;;) {
    checkWaterLevelAndPump();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// ================= MAIN TASK =================
void Task_Main(void *pv) {
  DHTData_t sensor;

  for (;;) {
    if (xQueueReceive(dhtQueue, &sensor, portMAX_DELAY)) {

      float humidity = sensor.humi;
      float temperature = sensor.temp;

      Firebase.getString(firebaseData, "/doan1/controller/mode/state");
      mode = firebaseData.stringData();

      Firebase.setFloat(firebaseData, "/doan1/controller/humidity", humidity);
      Firebase.setFloat(firebaseData, "/doan1/controller/temperature", temperature);

      if (mode == "auto") {
        if (temperature > 28) {
          digitalWrite(FAN_PIN, HIGH);
          fanState = HIGH;
        } else {
          digitalWrite(FAN_PIN, LOW);
          fanState = LOW;
        }

        if (humidity > 70) {
          digitalWrite(HEATER_PIN, HIGH);
          heaterState = HIGH;
        } else {
          digitalWrite(HEATER_PIN, LOW);
          heaterState = LOW;
        }
      }

      if (digitalRead(BUTTON3_PIN) == HIGH) {
        servo1.write(StateFeeding ? 90 : 0);
        StateFeeding = !StateFeeding;
      }

      if (digitalRead(BUTTON5_PIN) == HIGH) {
        digitalWrite(PUMP_PIN, StatePumping ? LOW : HIGH);
        StatePumping = !StatePumping;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
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

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  config.host = "https://doan1-c4eac-default-rtdb.firebaseio.com/";
  config.signer.tokens.legacy_token = "YOUR_TOKEN";
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  dhtQueue = xQueueCreate(5, sizeof(DHTData_t));

  xTaskCreate(Task_DHT, "DHT", 2048, NULL, 2, NULL);
  xTaskCreate(Task_Pump, "PUMP", 2048, NULL, 2, NULL);
  xTaskCreate(Task_Main, "MAIN", 4096, NULL, 1, NULL);
}

void loop() {}
