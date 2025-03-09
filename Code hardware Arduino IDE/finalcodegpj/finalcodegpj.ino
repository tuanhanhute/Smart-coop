#include <DHT.h>
#include <FirebaseESP32.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int trigPin = 25;  
const int echoPin = 26;
// Ngưỡng khoảng cách từ cảm biến đến mực nước (cm)
const float pumpStartDistance = 4.0;  // Bắt đầu bơm khi khoảng cách từ cảm biến đến mực nước lớn hơn 4cm

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Địa chỉ I2C của màn hình LCD và kích thước (16x2)

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

DHT dht(DHTPIN, DHTTYPE);

FirebaseData firebaseData;

Servo servo1;
int fanState = LOW; 
int heaterState = LOW;
int StateFeeding = LOW;
int StatePumping = LOW;

String mode = "";
String heater = "";
String fan = "";
String State = "";
String StatePump = "";

bool button1State = false; 
bool button2State = false;
bool button3State = false;
bool button4State = false;
bool button5State = false; 
bool previousPumpState = false;

void setup() {
  Serial.begin(115200);
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(2, 0);
  lcd.print("Smart COOP");
  
  pinMode(FAN_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(FEEDING_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);
  pinMode(BUTTON5_PIN, INPUT_PULLUP);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  servo1.attach(FEEDING_PIN);

  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.println("Initializing Firebase");

  config.host ="https://doan1-c4eac-default-rtdb.firebaseio.com/";
  config.signer.tokens.legacy_token = "O5y6r8IZpdEW5HLgVq4BGWNWqUHXMtfBqE35AYW1";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}
void checkWaterLevelAndPump() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Đo thời gian nhận tín hiệu phản hồi tại chân Echo
  long duration = pulseIn(echoPin, HIGH);

  // Tính khoảng cách từ cảm biến đến mặt nước (cm)
  float distanceToWater = duration * 0.034 / 2;

 bool currentPumpState = distanceToWater > pumpStartDistance;

  // So sánh trạng thái hiện tại với trạng thái trước đó
  if (currentPumpState != previousPumpState) {
    if (currentPumpState) {
      Serial.println("Mực nước thấp. Yêu cầu bơm nước...");
      digitalWrite(PUMP_PIN, HIGH);
      lcd.setCursor(2, 0);
      lcd.print("Mode: Auto  ");
      lcd.setCursor(2, 1);
      lcd.print("PUMP: ON   ");
      Firebase.setString(firebaseData, "/doan1/controller/drinking/state", "ON");
    } else {
      Serial.println("Mực nước đủ cao. Ngừng bơm nước.");
      digitalWrite(PUMP_PIN, LOW);
      lcd.setCursor(2, 0);
      lcd.print("Mode: Auto  ");
      lcd.setCursor(2, 1);
      lcd.print("PUMP: STOP  ");
      Firebase.setString(firebaseData, "/doan1/controller/drinking/state", "OFF");
    }

    // Cập nhật trạng thái trước đó
    previousPumpState = currentPumpState;
  }

  Serial.print("Khoảng cách từ cảm biến đến mực nước: ");
  Serial.print(distanceToWater);
  Serial.println(" cm");
}

void loop(){
  
  Firebase.getString(firebaseData, "/doan1/controller/mode/state"); 
  mode = firebaseData.stringData(); 
  float humidity = dht.readHumidity(); 
  float temperature = dht.readTemperature(); 
  Firebase.getString(firebaseData, "/doan1/controller/heater/state"); 
  heater = firebaseData.stringData(); 
  Firebase.getString(firebaseData, "/doan1/controller/fan/state"); 
  fan = firebaseData.stringData();
  Firebase.getString(firebaseData, "/doan1/controller/feeding/state"); 
  State = firebaseData.stringData(); 
  Firebase.getString(firebaseData, "/doan1/controller/drinking/state"); 
  StatePump = firebaseData.stringData(); 
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C");
  
  Firebase.setFloat(firebaseData, "/doan1/controller/humidity", humidity); 
  Firebase.setFloat(firebaseData, "/doan1/controller/temperature", temperature); 
if(mode=="auto"){
  if(temperature > 28){ 
    if(fanState == LOW){ 
      digitalWrite(FAN_PIN, HIGH); 
      fanState = HIGH;
      lcd.setCursor(2, 0);
      lcd.print("Mode: Auto  ");
      lcd.setCursor(2, 1); 
      lcd.print("FAN: ON   "); 
      Serial.println("Fan turned on");
      Firebase.setString(firebaseData, "/doan1/controller/fan/state", "ON");
    }
  }else{
    if(fanState == HIGH){ 
    digitalWrite(FAN_PIN, LOW); 
    fanState = LOW;
      lcd.setCursor(2, 0);
      lcd.print("Mode: Auto  ");
      lcd.setCursor(2, 1); 
      lcd.print("FAN: OFF   "); 
    Serial.println("Fan turned off");
    Firebase.setString(firebaseData, "/doan1/controller/fan/state", "OFF"); 
  }
  }
  if (humidity > 70) { 
    if(heaterState == LOW){ 
      digitalWrite(HEATER_PIN, HIGH); 
      heaterState = HIGH;
      lcd.setCursor(2, 0);
      lcd.print("Mode: Auto  ");
      lcd.setCursor(2, 1); 
      lcd.print("HEATER: ON   "); 
      Serial.println("Heater turned on");
      Firebase.setString(firebaseData, "/doan1/controller/heater/state", "ON"); 
    }
  } else{ 
    if(heaterState == HIGH){ 
      digitalWrite(HEATER_PIN, LOW); 
      heaterState = LOW;
      lcd.setCursor(2, 0);
      lcd.print("Mode: Auto  ");
      lcd.setCursor(2, 1); 
      lcd.print("HEATER: OFF   "); 
      Serial.println("Heater turned off");
      Firebase.setString(firebaseData, "/doan1/controller/heater/state", "OFF"); 
    }
  }
  if(State == "ON"){ 
      if(StateFeeding == LOW){ 
          digitalWrite(FEEDING_PIN, HIGH); 
          StateFeeding = HIGH;
          servo1.write(0);
          lcd.setCursor(2, 0);
          lcd.print("Mode: Auto  ");
          lcd.setCursor(2, 1); 
          lcd.print("FEEDING: ON   ");  
          Serial.println("Feeding...");
          Firebase.setString(firebaseData, "/doan1/controller/feeding/state", "ON");  
        }
    }else if(State == "OFF"){
          if (StateFeeding == HIGH){
          digitalWrite(FEEDING_PIN, LOW); 
          StateFeeding = LOW;
          servo1.write(90);
          lcd.setCursor(2, 0);
          lcd.print("Mode: Auto  ");
          lcd.setCursor(2, 1); 
          lcd.print("FEEDING: STOP   ");  
          Serial.println("Stop Feeding");
          Firebase.setString(firebaseData, "/doan1/controller/feeding/state", "OFF"); 
  } 
  }
  if(digitalRead(BUTTON3_PIN) == HIGH){
    if(StateFeeding == LOW){
        digitalWrite(FEEDING_PIN, HIGH);
        StateFeeding = HIGH;
        servo1.write(0);
        lcd.setCursor(2, 0);
        lcd.print("Mode: User  ");
        lcd.setCursor(2, 1); 
        lcd.print("FEEDING: ON   ");  
        Serial.println("Feeding...");
        Firebase.setString(firebaseData, "/doan1/controller/feeding/state", "ON"); 

    }else{
        digitalWrite(FEEDING_PIN, LOW);
        StateFeeding = LOW;
        servo1.write(90);
        lcd.setCursor(2, 0);
        lcd.print("Mode: User  ");
        lcd.setCursor(2, 1); 
        lcd.print("FEEDING: STOP   ");  
        Serial.println("Stop Feeding");
        Firebase.setString(firebaseData, "/doan1/controller/feeding/state", "OFF");
  }
  }
  checkWaterLevelAndPump();
  // if(StatePump == "ON"){ 
  //     if(StatePumping == LOW){ 
  //         digitalWrite(PUMP_PIN, HIGH); 
  //         StatePumping = HIGH;
  //         lcd.setCursor(2, 0);
  //         lcd.print("Mode: Auto  ");
  //         lcd.setCursor(2, 1); 
  //         lcd.print("PUMPING: ON   ");  
  //         Serial.println("Pump...");
  //         Firebase.setString(firebaseData, "/doan1/controller/drinking/state", "ON"); 
  //       }
  //   }else if(StatePump == "OFF"){
  //         if (StatePumping == HIGH){
  //         digitalWrite(PUMP_PIN, LOW); 
  //         StatePumping = LOW;
  //         lcd.setCursor(2, 0);
  //         lcd.print("Mode: Auto  ");
  //         lcd.setCursor(2, 1); 
  //         lcd.print("PUMPING: STOP   ");
  //         Serial.println("Stop Pump");
  //         Firebase.setString(firebaseData, "/doan1/controller/drinking/state", "OFF"); 
  // } 
  // }
  if(digitalRead(BUTTON5_PIN) == HIGH){
    if(StatePumping == LOW){
          digitalWrite(PUMP_PIN, HIGH);
          StatePumping = HIGH;
          lcd.setCursor(2, 0);
          lcd.print("Mode: User  ");
          lcd.setCursor(2, 1); 
          lcd.print("PUMPING: ON   "); 
          Serial.println("Pump...");
          Firebase.setString(firebaseData, "/doan1/controller/drinking/state", "ON");

    }else{
          digitalWrite(PUMP_PIN, LOW);
          StatePumping = LOW;
          lcd.setCursor(2, 0);
          lcd.print("Mode: User  ");
          lcd.setCursor(2, 1); 
          lcd.print("PUMPING: STOP   ");
          Serial.println("Stop Pump");
          Firebase.setString(firebaseData, "/doan1/controller/drinking/state", "OFF"); 
        }
    }
}
else if(mode=="user"){
  if(fan == "ON"){ 
    if(fanState == LOW){ 
          digitalWrite(FAN_PIN, HIGH); 
          fanState = HIGH;
          lcd.setCursor(2, 0);
          lcd.print("Mode: User");
          lcd.setCursor(2, 1); 
          lcd.print("FAN: ON   "); 
          Serial.println("Fan turned on");
          Firebase.setString(firebaseData, "/doan1/controller/fan/state", "ON"); 
  }
  }else if(fan == "OFF"){ 
    if(fanState == HIGH){ 
          digitalWrite(FAN_PIN, LOW); 
          fanState = LOW;
          lcd.setCursor(2, 0);
          lcd.print("Mode: User");
          lcd.setCursor(2, 1); 
          lcd.print("FAN: OFF   "); 
          Serial.println("Fan turned off");
          Firebase.setString(firebaseData, "/doan1/controller/fan/state", "OFF"); 
        }
  }
    if(heater == "ON"){ 
      if(heaterState == LOW){ 
          digitalWrite(HEATER_PIN, HIGH); 
          heaterState = HIGH;
          lcd.setCursor(2, 0);
          lcd.print("Mode: User");
          lcd.setCursor(2, 1); 
          lcd.print("HEATER: ON   "); 
          Serial.println("Heater turned on");
          Firebase.setString(firebaseData, "/doan1/controller/heater/state", "ON"); 
        }
    }else if(heater == "OFF"){
          if (heaterState == HIGH){
          digitalWrite(HEATER_PIN, LOW); 
          heaterState = LOW;
          lcd.setCursor(2, 0);
          lcd.print("Mode: User");
          lcd.setCursor(2, 1); 
          lcd.print("HEATER: OFF   "); 
          Serial.println("Heater turned off");
          Firebase.setString(firebaseData, "/doan1/controller/heater/state", "OFF");
  }
  } 
  if(State == "ON"){ 
      if(StateFeeding == LOW){ 
          digitalWrite(FEEDING_PIN, HIGH); 
          StateFeeding = HIGH;
          servo1.write(0);
          lcd.setCursor(2, 0);
          lcd.print("Mode: User");
          lcd.setCursor(2, 1); 
          lcd.print("FEEDING: ON   ");  
          Serial.println("Feeding...");
          Firebase.setString(firebaseData, "/doan1/controller/feeding/state", "ON");  
        }
    }else if(State == "OFF"){
          if (StateFeeding == HIGH){
          digitalWrite(FEEDING_PIN, LOW); 
          StateFeeding = LOW;
          servo1.write(90);
          lcd.setCursor(2, 0);
          lcd.print("Mode: User");
          lcd.setCursor(2, 1); 
          lcd.print("FEEDING: STOP   ");  
          Serial.println("Stop Feeding");
          Firebase.setString(firebaseData, "/doan1/controller/feeding/state", "OFF"); 
  } 
  }
  ///////////
  if(StatePump == "ON"){ 
      if(StatePumping == LOW){ 
          digitalWrite(PUMP_PIN, HIGH); 
          StatePumping = HIGH;
          lcd.setCursor(2, 0);
          lcd.print("Mode: User");
          lcd.setCursor(2, 1); 
          lcd.print("PUMP: ON   ");  
          Serial.println("Pump...");
          Firebase.setString(firebaseData, "/doan1/controller/drinking/state", "ON"); 
        }
    }else if(StatePump == "OFF"){
          if (StatePumping == HIGH){
          digitalWrite(PUMP_PIN, LOW); 
          StatePumping = LOW;
          lcd.setCursor(2, 0);
          lcd.print("Mode: User");
          lcd.setCursor(2, 1); 
          lcd.print("PUMP: STOP   "); 
          Serial.println("Stop Pump");
          Firebase.setString(firebaseData, "/doan1/controller/drinking/state", "OFF"); 
  } 
  }
    if (digitalRead(BUTTON1_PIN) == HIGH){
      //lcd.clear();
      //lcd.print("Bat quat");
      if(fanState == LOW){
        digitalWrite(FAN_PIN, HIGH);
        fanState = HIGH;
        lcd.setCursor(2, 0);
        lcd.print("Mode: User");
        lcd.setCursor(2, 1); 
        lcd.print("FAN: ON   "); 
        Serial.println("Fan turned on");
        Firebase.setString(firebaseData, "/doan1/controller/fan/state", "ON"); 
      }else{
        digitalWrite(FAN_PIN, LOW);
        fanState = LOW;
        lcd.setCursor(2, 0);
        lcd.print("Mode: User");
        lcd.setCursor(2, 1); 
        lcd.print("FAN: OFF   "); 
        Serial.println("Fan turned off");
        Firebase.setString(firebaseData, "/doan1/controller/fan/state", "OFF");
            //lcd.clear();
            //lcd.print("Tat den");a
      }
  }
    if(digitalRead(BUTTON2_PIN) == HIGH){
        //lcd.clear();
        //lcd.print("Bat den");
      if(heaterState == LOW){
        digitalWrite(HEATER_PIN, HIGH);
        heaterState = HIGH;
        Serial.println("Heater turned on");
        lcd.setCursor(2, 0);
        lcd.print("Mode: User");
        lcd.setCursor(2, 1); 
        lcd.print("HEATER: ON   "); 
        Firebase.setString(firebaseData, "/doan1/controller/heater/state", "ON");
        }else{
        digitalWrite(HEATER_PIN, LOW);
        heaterState = LOW;
        lcd.setCursor(2, 0);
        lcd.print("Mode: User");
        lcd.setCursor(2, 1); 
        lcd.print("HEATER: OFF   "); 
        Serial.println("Heater turned off");
        Firebase.setString(firebaseData, "/doan1/controller/heater/state", "OFF");
        }
        }
        if(digitalRead(BUTTON3_PIN) == HIGH){
        if(StateFeeding == LOW){
        digitalWrite(FEEDING_PIN, HIGH);
        StateFeeding = HIGH;
        servo1.write(0);
        lcd.setCursor(2, 0);
        lcd.print("Mode: User");
        lcd.setCursor(2, 1); 
        lcd.print("FEEDING: ON   ");  
        Serial.println("Feeding...");
        Firebase.setString(firebaseData, "/doan1/controller/feeding/state", "ON"); 
        }else{
        digitalWrite(FEEDING_PIN, LOW);
        StateFeeding = LOW;
        servo1.write(90);
        lcd.setCursor(2, 0);
        lcd.print("Mode: User");
        lcd.setCursor(2, 1); 
        lcd.print("FEEDING: OFF   ");  
        Serial.println("Stop Feeding");
        Firebase.setString(firebaseData, "/doan1/controller/feeding/state", "OFF");
        }
        }
        ///////////
        if(digitalRead(BUTTON5_PIN) == HIGH){
        if(StatePumping == LOW){
        digitalWrite(PUMP_PIN, HIGH);
        StatePumping = HIGH;
        lcd.setCursor(2, 0);
        lcd.print("Mode: User");
        lcd.setCursor(2, 1); 
        lcd.print("PUMP: ON   "); 
        Serial.println("Pump...");
        Firebase.setString(firebaseData, "/doan1/controller/drinking/state", "ON");
        }else{
        digitalWrite(PUMP_PIN, LOW);
        StatePumping = LOW;
        lcd.setCursor(2, 0);
        lcd.print("Mode: User");
        lcd.setCursor(2, 1); 
        lcd.print("PUMP: STOP   "); 
        Serial.println("Stop Pump");
        Firebase.setString(firebaseData, "/doan1/controller/drinking/state", "OFF"); 
        }
      }
    delay(2000);
  }
if (digitalRead(BUTTON4_PIN) == HIGH) {
  if (mode == "auto") {
    mode = "user";
    Firebase.setString(firebaseData, "/doan1/mode", "user");
        lcd.setCursor(2, 0);
        lcd.print("Mode: User");
    Serial.println("Switched to user mode");
  } else {
    mode = "auto";
    Firebase.setString(firebaseData, "/doan1/mode", "auto");
        lcd.setCursor(2, 0);
        lcd.print("Mode: Auto");
    Serial.println("Switched to auto mode");
  }
  delay(500); 
}

}
