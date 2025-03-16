#include <DHT.h>
#include <FirebaseESP32.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define WIFI_SSID "Nha xe C"
#define WIFI_PASSWORD "88888888"
#define DHTPIN 5     
#define DHTTYPE DHT11  
#define TRIG_PIN 25  
#define ECHO_PIN 26  
#define PUMP_THRESHOLD 4.0

FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo1;
String mode = "auto";

struct Device {
    int pin;
    bool state;
    String firebasePath;
    String label;
};

Device devices[] = {
    {12, LOW, "/doan1/controller/fan/state", "FAN"},
    {14, LOW, "/doan1/controller/heater/state", "HEATER"},
    {27, LOW, "/doan1/controller/feeding/state", "FEEDING"},
    {32, LOW, "/doan1/controller/drinking/state", "PUMP"}
};
const int deviceCount = sizeof(devices) / sizeof(devices[0]);

int buttons[] = {16, 17, 18, 19, 33};
const int buttonCount = sizeof(buttons) / sizeof(buttons[0]);
bool prevPumpState = false;

void updateDeviceState(Device &device, bool newState) {
    if (newState != device.state) {
        device.state = newState;
        digitalWrite(device.pin, newState);
        Firebase.setString(firebaseData, device.firebasePath, newState ? "ON" : "OFF");
        Serial.println(device.label + (newState ? " ON" : " OFF"));
        lcd.setCursor(2, 1);
        lcd.print(device.label + (newState ? ": ON  " : ": OFF "));
    }
}
void checkWaterLevelAndPump() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = duration * 0.034 / 2;
    bool pumpActive = distance > PUMP_THRESHOLD;
    
    if (pumpActive != prevPumpState) {
        prevPumpState = pumpActive;
        updateDeviceState(devices[3], pumpActive);
    }
}
void updateFirebaseData() {
    Firebase.getString(firebaseData, "/doan1/controller/mode/state"); 
    mode = firebaseData.stringData(); 
}

void updateTemperatureHumidity() {
    float humidity = dht.readHumidity(); 
    float temperature = dht.readTemperature(); 
    Serial.printf("Humidity: %.1f%% | Temperature: %.1f°C\n", humidity, temperature);
    Firebase.setFloat(firebaseData, "/doan1/controller/humidity", humidity); 
    Firebase.setFloat(firebaseData, "/doan1/controller/temperature", temperature); 
}
void setup() {
    Serial.begin(115200);
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(2, 0);
    lcd.print("Smart COOP");

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    
    for (int i = 0; i < deviceCount; i++) {
        pinMode(devices[i].pin, OUTPUT);
    }
    for (int i = 0; i < buttonCount; i++) {
        pinMode(buttons[i], INPUT_PULLUP);
    }
    
    servo1.attach(devices[2].pin);
    dht.begin();
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    
    config.host = "https://doan1-c4eac-default-rtdb.firebaseio.com/";
    config.signer.tokens.legacy_token = "O5y6r8IZpdEW5HLgVq4BGWNWqUHXMtfBqE35AYW1";
    
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}
void loop() {
    updateFirebaseData();
    updateTemperatureHumidity();

    if (mode == "auto") {
        updateDeviceState(devices[0], dht.readTemperature() > 28);  // FAN
        updateDeviceState(devices[1], dht.readHumidity() > 70);     // HEATER
        checkWaterLevelAndPump();
    } else if (mode == "user") {
        for (int i = 0; i < buttonCount; i++) {
            if (digitalRead(buttons[i]) == HIGH) {
                updateDeviceState(devices[i], !devices[i].state);
                if (i == 2) servo1.write(devices[i].state ? 0 : 90); // Điều khiển Servo
            }
        }
    }
    if (digitalRead(buttons[3]) == HIGH) {
        mode = (mode == "auto") ? "user" : "auto";
        Firebase.setString(firebaseData, "/doan1/mode", mode);
        lcd.setCursor(2, 0);
        lcd.print("Mode: " + mode);
        Serial.println("Switched to mode: " + mode);
        delay(500);
    }

    delay(2000);
}
