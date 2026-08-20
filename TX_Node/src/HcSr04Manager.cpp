#include "HcSr04Manager.h"

#ifdef USE_HC_SR04

// =========================================================================
// HcSr04Manager — จัดการเซนเซอร์วัดระยะอัลตร้าโซนิค HC-SR04 (Node 3)
// =========================================================================
// ฮาร์ดแวร์:
//   Trig Pin : D12 (Output)
//   Echo Pin : D13 (Input)
//   พิสัยวัด : 2 cm – 400 cm (ความแม่นยำสูงในระยะใกล้)
//   ความเร็วเสียงในอากาศ: ~343 m/s (0.0343 cm/us)
// =========================================================================

void initHcSr04() {
  pinMode(PIN_HC_TRIG, OUTPUT);
  pinMode(PIN_HC_ECHO, INPUT);

  // ตั้งสถานะเริ่มต้น Trig เป็น LOW
  digitalWrite(PIN_HC_TRIG, LOW);

  Serial.println(F("[+] HC-SR04 Ultrasonic Initialized"));
  Serial.print(F("[+] Trig Pin: D"));
  Serial.print(PIN_HC_TRIG);
  Serial.print(F(", Echo Pin: D"));
  Serial.println(PIN_HC_ECHO);
}

bool readHcSr04WaterLevel(uint16_t &waterLevelCm) {
  const uint8_t MAX_RETRIES = 3;

  for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++) {
    // 1. ส่งสัญญาณ Trigger Pulse (LOW 2us -> HIGH 10us -> LOW)
    digitalWrite(PIN_HC_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_HC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_HC_TRIG, LOW);

    // 2. วัดระยะเวลาที่สัญญาณ Echo เป็น HIGH (Timeout 25,000 us ~= 4 เมตร)
    unsigned long durationUs = pulseIn(PIN_HC_ECHO, HIGH, 25000UL);

    if (durationUs > 0) {
      // ระยะทาง (cm) = (durationUs * 0.0343) / 2
      float distance = (durationUs * 0.0343) / 2.0;

      // ตรวจสอบพิสัยของ HC-SR04 (2cm - 400cm)
      if (distance >= 2.0 && distance <= 400.0) {
        waterLevelCm = (uint16_t)round(distance);

        Serial.print(F("[HC-SR04] OK: "));
        Serial.print(waterLevelCm);
        Serial.println(F("cm"));

        return true;
      }
    }

    // หากอ่านไม่สำเร็จ ให้หน่วง 50ms ก่อนลองใหม่
    if (attempt < MAX_RETRIES - 1) {
      delay(50);
    }
  }

  Serial.println(F("[HC-SR04] ERROR: Timeout or Out of Range"));
  return false;
}

#endif // USE_HC_SR04
