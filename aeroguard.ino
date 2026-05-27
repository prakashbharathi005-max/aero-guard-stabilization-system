#include <Wire.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include "ThingSpeak.h"

// ===== SERVO =====
Servo servoX;
Servo servoY;
Servo servoZ;

// ===== WIFI =====
const char* ssid = "redmi";
const char* password = "00000000";

// ===== THINGSPEAK =====
WiFiClient client;
unsigned long channelNumber = 3308457;
const char * writeAPIKey = "SFA4CPRLUJWVACZP";

// ===== MPU =====
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;

// ===== FILTER =====
float angleX = 0;
float angleY = 0;
float dt;
unsigned long prevTime;

// ===== PID =====
float setpoint = 0;

float errorX, prevErrorX = 0, integralX = 0, outputX;
float errorY, prevErrorY = 0, integralY = 0, outputY;

float Kp = 1.5;
float Ki = 0.02;
float Kd = 0.8;

void setup() {
  Serial.begin(9600);   // ✅ Stable baud
  Wire.begin();

  // Servo pins
  servoX.attach(14); // D5
  servoY.attach(12); // D6
  servoZ.attach(13); // D7

  // ===== WIFI SAFE CONNECT =====
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected");
  } else {
    Serial.println("\nWiFi Failed → Running Offline");
  }

  ThingSpeak.begin(client);

  // Wake MPU6050
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission();

  prevTime = millis();
}

void loop() {

  // ===== READ MPU =====
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)0x68, (size_t)14, true);

  AcX = Wire.read()<<8 | Wire.read();
  AcY = Wire.read()<<8 | Wire.read();
  AcZ = Wire.read()<<8 | Wire.read();
  Wire.read(); Wire.read();
  GyX = Wire.read()<<8 | Wire.read();
  GyY = Wire.read()<<8 | Wire.read();
  GyZ = Wire.read()<<8 | Wire.read();

  float accX = AcX / 16384.0;
  float accY = AcY / 16384.0;
  float accZ = AcZ / 16384.0;

  float gyroX = GyX / 131.0;
  float gyroY = GyY / 131.0;

  // ===== TIME =====
  unsigned long currTime = millis();
  dt = (currTime - prevTime) / 1000.0;
  prevTime = currTime;

  // ===== ANGLES =====
  float accAngleX = atan2(accY, accZ) * 180 / PI;
  float accAngleY = atan2(-accX, accZ) * 180 / PI;

  // ===== FILTER =====
  angleX = 0.98 * (angleX + gyroX * dt) + 0.02 * accAngleX;
  angleY = 0.98 * (angleY + gyroY * dt) + 0.02 * accAngleY;

  // ===== PID X =====
  errorX = setpoint - angleX;
  integralX += errorX * dt;
  float derivativeX = (errorX - prevErrorX) / dt;
  outputX = Kp*errorX + Ki*integralX + Kd*derivativeX;
  prevErrorX = errorX;

  // ===== PID Y =====
  errorY = setpoint - angleY;
  integralY += errorY * dt;
  float derivativeY = (errorY - prevErrorY) / dt;
  outputY = Kp*errorY + Ki*integralY + Kd*derivativeY;
  prevErrorY = errorY;

  // ===== SERVO CONTROL =====
  servoX.write(constrain(90 + outputX, 0, 180));
  servoY.write(constrain(90 + outputY, 0, 180));
  servoZ.write(90);

  // ===== SERIAL FOR MATLAB (ONLY THIS FORMAT) =====
  Serial.print(angleX);
  Serial.print(",");
  Serial.println(angleY);

  // ===== THINGSPEAK (SAFE) =====
  static unsigned long lastUpload = 0;

  if (WiFi.status() == WL_CONNECTED && millis() - lastUpload > 15000) {
    ThingSpeak.setField(1, angleX);
    ThingSpeak.setField(2, angleY);
    ThingSpeak.writeFields(channelNumber, writeAPIKey);
    lastUpload = millis();
  }

  delay(50);   // fast update for MATLAB
}