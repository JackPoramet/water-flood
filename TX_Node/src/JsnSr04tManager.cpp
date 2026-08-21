#include "JsnSr04tManager.h"

#ifdef USE_JSN_SR04T

// =========================================================================
// JsnSr04tManager — ไดรเวอร์ตรวจจับพัลส์อัตโนมัติสำหรับ JSN-SR04T (Node 2)
// ตรวจสอบทั้งคู่พิน (D5/D12 และ D12/D5) เพื่อหาคู่ที่ต่อสายจริงโดยอัตโนมัติ
// =========================================================================

static uint8_t activeTrig = PIN_JSN_TRIG;
static uint8_t activeEcho = PIN_JSN_ECHO;
static bool autoDetected = false;

static unsigned long singlePing(uint8_t trigPin, uint8_t echoPin) {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // ส่ง Trigger Pulse 20us
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(20);
  digitalWrite(trigPin, LOW);

  return pulseIn(echoPin, HIGH, 45000UL);
}

void initJsnSr04t() {
  pinMode(PIN_JSN_TRIG, OUTPUT);
  pinMode(PIN_JSN_ECHO, INPUT);
  digitalWrite(PIN_JSN_TRIG, LOW);

  Serial.println(F("[+] JSN-SR04T Initialized with Auto-Pin Diagnostic"));
  Serial.print(F("    Default -> Trig: D"));
  Serial.print(PIN_JSN_TRIG);
  Serial.print(F(", Echo: D"));
  Serial.println(PIN_JSN_ECHO);
}

bool readJsnSr04tWaterLevel(uint16_t &waterLevelCm) {
  unsigned long pulseWidth = 0;

  // 1. ถ้าตรวจพบคู่พินที่ถูกต้องแล้ว ให้ใช้คู่นั้นยิงปกติ
  if (autoDetected) {
    pulseWidth = singlePing(activeTrig, activeEcho);
    if (pulseWidth == 0) {
      delay(50);
      pulseWidth = singlePing(activeTrig, activeEcho);
    }
  } else {
    // 2. ถ้ายังไม่ล็อคคู่พิน ให้ทดสอบทั้ง 2 ทิศทาง (D5->D12 และ D12->D5)
    // ทดสอบแบบที่ 1: Trig=12, Echo=5
    unsigned long pulseA = singlePing(12, 5);
    delay(40);

    // ทดสอบแบบที่ 2: Trig=5, Echo=12
    unsigned long pulseB = singlePing(5, 12);

    if (pulseA > 0) {
      activeTrig = 12;
      activeEcho = 5;
      autoDetected = true;
      pulseWidth = pulseA;
      Serial.println(F("[+] Auto-Detected: Trig=D12, Echo=D5!"));
    } else if (pulseB > 0) {
      activeTrig = 5;
      activeEcho = 12;
      autoDetected = true;
      pulseWidth = pulseB;
      Serial.println(F("[+] Auto-Detected: Trig=D5, Echo=D12!"));
    } else {
      Serial.println(F("[JSN-SR04T] No pulse on (D12/D5) or (D5/D12) -> Check 5V power and transducer cable"));
      return false;
    }
  }

  if (pulseWidth > 0) {
    unsigned int distance = (unsigned int)(pulseWidth * 0.0173681);

    if (distance >= 15 && distance <= 600) {
      waterLevelCm = (uint16_t)distance;

      Serial.print(F("PulseWidth: "));
      Serial.print(pulseWidth);
      Serial.print(F(" us -> Distance is "));
      Serial.print(distance);
      Serial.println(F(" cm."));

      return true;
    }
  }

  return false;
}

#endif // USE_JSN_SR04T
