#ifndef ESP32_INCLUDE_PLACEOCCUPANCYSTATEMACHINE_HPP_
#define ESP32_INCLUDE_PLACEOCCUPANCYSTATEMACHINE_HPP_

#include "secrets.hpp"
#include <Arduino.h>
#include <WiFi.h>
#include <vector>

class OccupancyStateMachine {
public:
  enum State { Start, Init, Calibrate, Scan, Upload, Idle };

  OccupancyStateMachine(const std::vector<uint8_t> &ledPins,
                        const uint8_t servoPin, uint8_t sonarPin,
                        const float anglePerSeat,
                        const uint32_t scanIntervalSec,
                        const uint8_t totalSeats = 4,
                        const float minimumDistanceFromSonarCm = 25.0f)
      : ledPins_(ledPins), servoPin_(servoPin), sonarPin_(sonarPin),
        anglePerSeat_(anglePerSeat), scanIntervalSec_(scanIntervalSec),
        totalSeats_(totalSeats), emptySeats_(0),
        minimumDistanceFromSonar_(minimumDistanceFromSonarCm), state_(Start) {};

  void begin() {
    pinMode(sonarPin_, INPUT_PULLUP);

    for (uint8_t pin : ledPins_) {
      pinMode(pin, OUTPUT);
    }
  }

private:
  const std::vector<uint8_t> ledPins_;
  const uint8_t servoPin_;
  const uint8_t sonarPin_;
  const float anglePerSeat_;
  const uint32_t scanIntervalSec_;
  const uint8_t totalSeats_;
  const uint8_t emptySeats_;
  State state_;
  bool systemReady_ = false;
  const float minimumDistanceFromSonar_;

  const int servoChannel_ = 0;
  const int servoMinPwm_ = 3276; // ~1ms
  const int servoMaxPwm_ = 6553; // ~2ms

  void setServoAngle(uint8_t angle) const {
    angle = constrain(angle, 0, 180);
    int pwmValue = map(angle, 0, 180, servoMinPwm_, servoMaxPwm_);
    ledcWrite(servoChannel_, pwmValue);
  }

  void runStateMachine() {
    switch (state_) { // states
    case Start:
      Serial.print(F("Start -> Init"));
      state_ = Init;
      break;
    case Init:
      Serial.print(F("Init -> Calibrate"));
      break;
    case Calibrate:
      break;
    case Scan:
      break;
    case Upload:
      break;
    case Idle:
      break;
    }

    switch (state_) { // actions
    case Start:
      break;
    case Init:
      systemReady_ = connectToWifi() && initSensors();
      Serial.print(F("Init -> Calibrate"));
      break;
    case Calibrate:
      break;
    case Scan:
      break;
    case Upload:
      break;
    case Idle:
      break;
    }
  }

  static bool connectToWifi() {
    WiFiClass::mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println(F("\nConnecting to WiFi Network .."));

    uint8_t attempts = 0;
    const uint8_t MAX_ATTEMPTS = 20;
    while (WiFiClass::status() != WL_CONNECTED && attempts < MAX_ATTEMPTS) {
      attempts++;
      Serial.print(".");
      delay(100);
    }

    if (attempts >= MAX_ATTEMPTS) {
      return false;
    }

    Serial.println(F("\nConnected to the WiFi network"));
    Serial.print(F("Local ESP32 IP: "));
    Serial.println(WiFi.localIP());

    return true;
  }

  float readSonarData() const {
    // 147 us per inch OR 58 us per cm
    unsigned long pulse = pulseIn(sonarPin_, HIGH);

    return pulse / 58.0;
  }

  bool initSensors() {
    Serial.println("Initializing hardware...");

    ledcSetup(servoChannel_, 50, 16);
    ledcAttachPin(servoPin_, servoChannel_);
    setServoAngle(0);
    Serial.println("Servo OK.");

    float dist = readSonarData();
    if (dist < minimumDistanceFromSonar_) {
      Serial.print(F("[INIT] ERROR: Sonar too close to object"));
      return false;
    }

    return true;
  }
};
#endif
