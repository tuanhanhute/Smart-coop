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

// ================= FIREBASE =================
FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;

// ================= DEVICE =================
DHT dht(DHTPIN, DHTTYPE);
Servo servo1;

// ================= STATE =================
String mode, heater, fan, State, StatePump;

int fanState = LOW;
int heaterState = LOW;
int StateFeeding = LOW;
int StatePumping = LOW;

bool previousPumpState = false;

// ================= SENSOR QUEUE =================
typedef struct {
  float temp;
  float humi;
} DHTData_t;

QueueHandle_t dhtQueue;

// ================= LCD =================
void lcdShow(const char *l1, const char *l2) {
  lcd.setCursor(2, 0);
  lcd.print(l1);
  lcd.print("   ");

  lcd.setCursor(2, 1);
  lcd.print(l2);
  lcd.print("   ");
}

// ================= ULTRASONIC =================
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
    if (currentPumpState) {
      digitalWrite(PUMP_PIN, HIGH);
      lcdShow("Mode: Auto", "PUMP: ON");
      Firebase.setString(firebaseData, "/doan1/controller/drinking/state", "ON");
    } else {
      digitalWrite(PUMP_PIN, LOW);
      lcdShow("Mode: Auto", "PUMP: OFF");
      Firebase.setString(firebaseData, "/doan1/controller/drinking/state", "OFF");
    }
    previousPumpState = currentPumpState;
  }
}

// ================= TASK DHT =================
void Task_DHT(void *pv) {
  DHTData_t data;

  for (;;) {
    data.humi = dht.readHumidity();
    data.temp = dht.readTemperature();

    xQueueSend(dhtQueue, &data, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// ================= TASK BUTTON MODE SWITCH =================
void Task_Mode(void *pv) {
  for (;;) {
    if (digitalRead(BUTTON4_PIN) == HIGH) {
      vTaskDelay(pdMS_TO_TICKS(200)); 

      if (mode == "auto") {
        mode = "user";
        Firebase.setString(firebaseData, "/doan1/mode", "user");
        lcdShow("Mode: User", "");
      } else {
        mode = "auto";
        Firebase.setString(firebaseData, "/doan1/mode", "auto");
        lcdShow("Mode: Auto", "");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ================= TASK MAIN CONTROL =================
void Task_Main(void *pv) {
  DHTData_t sensor;

  for (;;) {

    Firebase.getString(firebaseData, "/doan1/controller/mode/state");
    mode = firebaseData.stringData();

    Firebase.getString(firebaseData, "/doan1/controller/fan/state");
    fan = firebaseData.stringData();

    Firebase.getString(firebaseData, "/doan1/controller/heater/state");
    heater = firebaseData.stringData();

    Firebase.getString(firebaseData, "/doan1/controller/feeding/state");
    State = firebaseData.stringData();

    Firebase.getString(firebaseData, "/doan1/controller/drinking/state");
    StatePump = firebaseData.stringData();

    if (xQueueReceive(dhtQueue, &sensor, portMAX_DELAY)) {

      float temperature = sensor.temp;
      float humidity = sensor.humi;

      Firebase.setFloat(firebaseData, "/doan1/controller/temperature", temperature);
      Firebase.setFloat(firebaseData, "/doan1/controller/humidity", humidity);

      // ================= AUTO MODE =================
      if (mode == "auto") {

        if (temperature > 28 && fanState == LOW) {
          digitalWrite(FAN_PIN, HIGH);
          fanState = HIGH;
          lcdShow("Mode: Auto", "FAN: ON");
        } else if (temperature <= 28 && fanState == HIGH) {
          digitalWrite(FAN_PIN, LOW);
          fanState = LOW;
          lcdShow("Mode: Auto", "FAN: OFF");
        }

        if (humidity > 70 && heaterState == LOW) {
          digitalWrite(HEATER_PIN, HIGH);
          heaterState = HIGH;
          lcdShow("Mode: Auto", "HEATER: ON");
        } else if (humidity <= 70 && heaterState == HIGH) {
          digitalWrite(HEATER_PIN, LOW);
          heaterState = LOW;
          lcdShow("Mode: Auto", "HEATER: OFF");
        }

        if (State == "ON" && StateFeeding == LOW) {
          digitalWrite(FEEDING_PIN, HIGH);
          servo1.write(0);
          StateFeeding = HIGH;
          lcdShow("Mode: Auto", "FEEDING: ON");
        } else if (State == "OFF" && StateFeeding == HIGH) {
          digitalWrite(FEEDING_PIN, LOW);
          servo1.write(90);
          StateFeeding = LOW;
          lcdShow("Mode: Auto", "FEEDING: OFF");
        }

        checkWaterLevelAndPump();
      }

      // ================= USER MODE =================
      else if (mode == "user") {

        // FAN 
        if (fan == "ON" && fanState == LOW) {
          digitalWrite(FAN_PIN, HIGH);
          fanState = HIGH;
          lcdShow("Mode: User", "FAN: ON");
        }
        if (fan == "OFF" && fanState == HIGH) {
          digitalWrite(FAN_PIN, LOW);
          fanState = LOW;
          lcdShow("Mode: User", "FAN: OFF");
        }

        if (digitalRead(BUTTON1_PIN) == HIGH) {
          fanState = !fanState;
          digitalWrite(FAN_PIN, fanState);
          Firebase.setString(firebaseData, "/doan1/controller/fan/state", fanState ? "ON" : "OFF");
          lcdShow("Mode: User", fanState ? "FAN: ON" : "FAN: OFF");
        }

        // HEATER
        if (heater == "ON" && heaterState == LOW) {
          digitalWrite(HEATER_PIN, HIGH);
          heaterState = HIGH;
          lcdShow("Mode: User", "HEATER: ON");
        }
        if (heater == "OFF" && heaterState == HIGH) {
          digitalWrite(HEATER_PIN, LOW);
          heaterState = LOW;
          lcdShow("Mode: User", "HEATER: OFF");
        }

        if (digitalRead(BUTTON2_PIN) == HIGH) {
          heaterState = !heaterState;
          digitalWrite(HEATER_PIN, heaterState);
          Firebase.setString(firebaseData, "/doan1/controller/heater/state", heaterState ? "ON" : "OFF");
        }

        // FEEDING
        if (State == "ON" && StateFeeding == LOW) {
          digitalWrite(FEEDING_PIN, HIGH);
          servo1.write(0);
          StateFeeding = HIGH;
          lcdShow("Mode: User", "FEEDING: ON");
        }
        if (State == "OFF" && StateFeeding == HIGH) {
          digitalWrite(FEEDING_PIN, LOW);
          servo1.write(90);
          StateFeeding = LOW;
          lcdShow("Mode: User", "FEEDING: OFF");
        }

        if (digitalRead(BUTTON3_PIN) == HIGH) {
          StateFeeding = !StateFeeding;
          digitalWrite(FEEDING_PIN, StateFeeding);
          servo1.write(StateFeeding ? 0 : 90);
          Firebase.setString(firebaseData, "/doan1/controller/feeding/state", StateFeeding ? "ON" : "OFF");
        }

        // PUMP
        if (StatePump == "ON" && StatePumping == LOW) {
          digitalWrite(PUMP_PIN, HIGH);
          StatePumping = HIGH;
          lcdShow("Mode: User", "PUMP: ON");
        }
        if (StatePump == "OFF" && StatePumping == HIGH) {
          digitalWrite(PUMP_PIN, LOW);
          StatePumping = LOW;
          lcdShow("Mode: User", "PUMP: OFF");
        }

        if (digitalRead(BUTTON5_PIN) == HIGH) {
          StatePumping = !StatePumping;
          digitalWrite(PUMP_PIN, StatePumping);
          Firebase.setString(firebaseData, "/doan1/controller/drinking/state", StatePumping ? "ON" : "OFF");
        }
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
  while (WiFi.status() != WL_CONNECTED) delay(500);

  config.host = "https://doan1-c4eac-default-rtdb.firebaseio.com/";
  config.signer.tokens.legacy_token = "O5y6r8IZpdEW5HLgVq4BGWNWqUHXMtfBqE35AYW1";
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  dhtQueue = xQueueCreate(5, sizeof(DHTData_t));

  xTaskCreate(Task_DHT, "DHT", 2048, NULL, 2, NULL);
  xTaskCreate(Task_Mode, "MODE", 2048, NULL, 3, NULL);  
  xTaskCreate(Task_Main, "MAIN", 4096, NULL, 1, NULL);
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}
