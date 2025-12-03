#ifndef ESP32_INCLUDE_PLACEOCCUPANCYSTATEMACHINE_HPP_
#define ESP32_INCLUDE_PLACEOCCUPANCYSTATEMACHINE_HPP_

#include <Arduino.h>
#include <vector>

class OccupancyStateMachine {
public:
  enum State { Start, Init, Calibrate, Scan, Upload, Idle };

  OccupancyStateMachine(const std::vector<uint8_t> &ledPins,
                        const uint8_t servoPin, uint8_t sonarPin,
                        const float anglePerSeat,
                        const uint32_t scanIntervalSec,
                        const uint8_t totalSeats = 4,
                        const float minimumDistanceFromSonarCm = 25.0f,
                        const uint8_t scanningLed = 2)
      : ledPins_(ledPins), servoPin_(servoPin), sonarPin_(sonarPin),
        anglePerSeat_(anglePerSeat), scanIntervalSec_(scanIntervalSec),
        scanIntervalMs_(scanIntervalSec * 1000UL), totalSeats_(totalSeats),
        emptySeats_(0), state_(Start),
        minimumDistanceFromSonar_(minimumDistanceFromSonarCm),
        scanningPin_(scanningLed) {}

  void begin() {
    pinMode(sonarPin_, INPUT_PULLUP);
    pinMode(scanningPin_, OUTPUT);
    digitalWrite(scanningPin_, LOW);

    for (uint8_t pin : ledPins_) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
    }

    baselines_.assign(totalSeats_, 0.0f);
    seatOccupied_.assign(totalSeats_, false);

    ledcSetup(servoChannel_, 50, 16); // 50 Hz, 16-bit PWM
    ledcAttachPin(servoPin_, servoChannel_);
    setServoAngle(0);

    lastScanMs_ = millis();
  }

  void update() {
    if (calibRequested_ && state_ != Calibrate) {
      state_ = Calibrate;
      calibrationDone_ = false;
      Serial.println(F("[SM] Entering Calibrate state"));
    }

    const unsigned long now = millis();
    runStateMachine(now);
  }

  uint8_t emptySeats() const { return emptySeats_; }

  State currentState() const { return state_; }

  void setCalibrationMode(bool enable) {
    calibRequested_ = enable;
    if (enable) {
      // we’ll do the transition to Calibrate in update()
      Serial.println(F("[SM] Calibration requested by server"));
    }
  }

  bool isCalibrating() const { return state_ == Calibrate; }

private:
  // -------- Config / pins --------
  const std::vector<uint8_t> ledPins_;
  const uint8_t servoPin_;
  const uint8_t sonarPin_;
  const uint8_t scanningPin_;
  const float anglePerSeat_;
  const uint32_t scanIntervalSec_;
  const uint32_t scanIntervalMs_;
  const uint32_t timeBetweenSeatScanMs_ = 3000; // 3 sec
  const uint8_t totalSeats_;
  uint8_t emptySeats_;
  State state_;
  bool hardwareReady_ = false;
  const float minimumDistanceFromSonar_;

  const int servoChannel_ = 0;
  const int servoMinPwm_ = 3276; // ~1ms
  const int servoMaxPwm_ = 6553; // ~2ms

  // -------- Runtime data --------
  std::vector<float> baselines_;
  std::vector<bool> seatOccupied_;

  bool initAttempted_ = false;
  bool calibrationDone_ = false;
  bool calibRequested_ = false;
  bool scanDone_ = false;

  unsigned long lastScanMs_ = 0;

  const float occupancyDeltaCm_ = 10.0f;

  // ---- Helpers ----
  void setServoAngle(uint8_t angle) const {
    angle = constrain(angle, 0, 180);
    int pwmValue = map(angle, 0, 180, servoMinPwm_, servoMaxPwm_);
    ledcWrite(servoChannel_, pwmValue);
  }

  float readSonarData() const {
    unsigned long pulse = pulseIn(sonarPin_, HIGH, 50000UL); // timeout 50 ms
    if (pulse == 0) {
      return 9999.0f;
    }
    return pulse / 58.0f; // µs -> cm
  }

  void setAllLeds(bool on) {
    for (uint8_t pin : ledPins_) {
      digitalWrite(pin, on ? HIGH : LOW);
    }
  }

  void updateSeatLedsFromOccupancy() {
    size_t n = min<size_t>(ledPins_.size(), seatOccupied_.size());
    for (size_t i = 0; i < n; ++i) {
      digitalWrite(ledPins_[i], seatOccupied_[i] ? LOW : HIGH);
    }
  }

  bool initSensors() {
    Serial.println(F("[INIT] Initializing hardware..."));

    setServoAngle(0);
    delay(300);
    Serial.println(F("[INIT] Servo OK"));

    float dist = readSonarData();
    if (dist < minimumDistanceFromSonar_) {
      Serial.println(F("[INIT] ERROR: Sonar too close to object"));
      return false;
    }

    Serial.println(F("[INIT] Sonar OK"));
    return true;
  }

  bool performCalibration() {
    Serial.println(F("[CALIB] Starting calibration..."));
    const int samplesPerSeat = 5;

    for (uint8_t seat = 0; seat < totalSeats_; ++seat) {
      float angle = anglePerSeat_ * seat;
      Serial.print(F("[CALIB] Seat "));
      Serial.print(seat);
      Serial.print(F(" angle="));
      Serial.println(angle);

      setServoAngle(static_cast<uint8_t>(angle));
      delay(300);

      float sum = 0.0f;
      for (int i = 0; i < samplesPerSeat; ++i) {
        float d = readSonarData();
        sum += d;
        delay(60);
      }
      float avg = sum / samplesPerSeat;
      baselines_[seat] = avg;

      Serial.print(F("        baseline="));
      Serial.print(avg);
      Serial.println(F(" cm"));
      delay(timeBetweenSeatScanMs_);
    }

    setServoAngle(0);
    Serial.println(F("[CALIB] Done."));
    return true;
  }

  bool performScan() {
    Serial.println(F("[SCAN] Scanning seats..."));
    const int samplesPerSeat = 3;

    emptySeats_ = 0;

    for (uint8_t seat = 0; seat < totalSeats_; ++seat) {
      float angle = anglePerSeat_ * seat;
      setServoAngle(static_cast<uint8_t>(angle));
      delay(300);

      float sum = 0.0f;
      for (int i = 0; i < samplesPerSeat; ++i) {
        float d = readSonarData();
        sum += d;
        delay(60);
      }
      float avg = sum / samplesPerSeat;

      bool occupied = avg < (baselines_[seat] - occupancyDeltaCm_);
      seatOccupied_[seat] = occupied;
      if (!occupied) {
        emptySeats_++;
      }

      Serial.print(F("[SCAN] Seat "));
      Serial.print(seat);
      Serial.print(F(" avg="));
      Serial.print(avg);
      Serial.print(F(" cm -> "));
      Serial.println(occupied ? F("OCCUPIED") : F("FREE"));
      delay(timeBetweenSeatScanMs_);
    }

    setServoAngle(0);
    updateSeatLedsFromOccupancy();

    Serial.print(F("[SCAN] Empty seats: "));
    Serial.println(emptySeats_);

    return true;
  }

  // ---- State machine ----
  void runStateMachine(unsigned long nowMs) {
    // --- Transitions ---
    switch (state_) {
    case Start:
      state_ = Init;
      break;

    case Init:
      if (hardwareReady_) {
        state_ = Calibrate;
        Serial.println(F("[SM] INIT -> CALIBRATE"));
      }
      break;

    case Calibrate:
      if (calibrationDone_) {
        lastScanMs_ = nowMs;
        state_ = Idle;
        Serial.println(F("[SM] CALIBRATE -> IDLE"));
      }

      break;

    case Idle:
      if (hardwareReady_ && calibrationDone_ &&
          (nowMs - lastScanMs_) >= scanIntervalMs_) {
        scanDone_ = false;
        state_ = Scan;
        Serial.println(F("[SM] IDLE -> SCAN"));
      }
      break;

    case Scan:
      if (scanDone_) {
        lastScanMs_ = nowMs;
        state_ = Idle;
        Serial.println(F("[SM] SCAN -> IDLE"));
      }
      break;

    case Upload:
      state_ = Idle;
      break;
    }

    // --- Actions ---
    switch (state_) {
    case Start:
      break;

    case Init:
      if (!initAttempted_) {
        setAllLeds(true);
        hardwareReady_ = initSensors();
        initAttempted_ = true;
        if (!hardwareReady_) {
          setAllLeds(false);
        }
      }
      break;

    case Calibrate:
      if (!calibrationDone_) {
        setAllLeds(false);
        calibrationDone_ = performCalibration();
      } else {
        calibRequested_ = false;
        scanDone_ = false;
      }
      break;

    case Idle:
      // LEDs show last scan result
      break;

    case Scan:
      if (!scanDone_) {
        digitalWrite(scanningPin_, HIGH);
        scanDone_ = performScan();
        digitalWrite(scanningPin_, LOW);
      }
      break;

    case Upload:
      // unused here
      break;
    }
  }
};

#endif
