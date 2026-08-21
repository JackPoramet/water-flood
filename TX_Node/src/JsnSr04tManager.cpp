#include "JsnSr04tManager.h"

#ifdef USE_JSN_SR04T

// =========================================================================
// JsnSr04tManager — จัดการเซนเซอร์วัดระยะอัลตร้าโซนิคกันน้ำ JSN-SR04T (Node 2)
// =========================================================================
// ขาต่อใช้งานบน LoRa32u4:
//   Trig Pin : D10 (Output) -> ส่งพัลส์ 20us
//   Echo Pin : D11 (Input)  -> รับพัลส์สะท้อนกลับ (Timeout 45,000us ~= 7.5 เมตร)
//   VCC      : 5V (USB Pin)
//   GND      : GND
// =========================================================================

void initJsnSr04t() {
  pinMode(PIN_JSN_TRIG, OUTPUT);
  pinMode(PIN_JSN_ECHO, INPUT);
  digitalWrite(PIN_JSN_TRIG, LOW);

  Serial.println(F("[+] JSN-SR04T Waterproof Ultrasonic Initialized"));
  Serial.print(F("    Trig Pin: D"));
  Serial.print(PIN_JSN_TRIG);
  Serial.print(F(", Echo Pin: D"));
  Serial.println(PIN_JSN_ECHO);
}

bool readJsnSr04tWaterLevel(uint16_t &waterLevelCm) {
  const uint8_t MAX_RETRIES = 3;

  for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++) {
    // 1. ส่งสัญญาณ Trigger Pulse 20us
    digitalWrite(PIN_JSN_TRIG, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN_JSN_TRIG, HIGH);
    delayMicroseconds(20);
    digitalWrite(PIN_JSN_TRIG, LOW);

    // 2. วัดความกว้างพัลส์สัญญาณ Echo (Timeout 45,000us ~= 7.5 เมตร)
    unsigned long pulseWidth = pulseIn(PIN_JSN_ECHO, HIGH, 45000UL);

    if (pulseWidth > 0) {
      // คำนวณระยะทาง: ระยะทาง (cm) = pulseWidth * 0.0173681
      unsigned int distance = (unsigned int)(pulseWidth * 0.0173681);

      // พิสัยของ JSN-SR04T คือ 20cm ถึง 600cm
      if (distance >= 20 && distance <= 600) {
        waterLevelCm = (uint16_t)distance;

        Serial.print(F("[JSN-SR04T] Pulse: "));
        Serial.print(pulseWidth);
        Serial.print(F(" us -> Distance: "));
        Serial.print(distance);
        Serial.println(F(" cm"));

        return true;
      }
    }

    if (attempt < MAX_RETRIES - 1) {
      delay(60); // เว้นระยะระหว่างการยิงพัลส์รอบใหม่
    }
  }

  Serial.println(F("[JSN-SR04T] ERROR: No echo response or out of range (<20cm or >600cm)"));
  return false;
}

#endif // USE_JSN_SR04T
