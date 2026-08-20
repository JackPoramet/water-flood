#include "JsnSr04tManager.h"

#ifdef USE_JSN_SR04T

// =========================================================================
// JsnSr04tManager — จัดการเซนเซอร์วัดระยะอัลตร้าโซนิค JSN-SR04T (Node 2)
// =========================================================================
// ฮาร์ดแวร์:
//   Trig Pin : D10 (Output)
//   Echo Pin : D11 (Input)
//   พิสัยวัด : 20 cm – 600 cm (Dead zone < 20 cm)
//   ความเร็วเสียงในอากาศ: ~343 m/s (0.0343 cm/us)
// =========================================================================

void initJsnSr04t() {
  pinMode(PIN_JSN_TRIG, OUTPUT);
  pinMode(PIN_JSN_ECHO, INPUT);

  // ตั้งสถานะเริ่มต้น Trig เป็น LOW
  digitalWrite(PIN_JSN_TRIG, LOW);

  Serial.println(F("[+] JSN-SR04T Ultrasonic Initialized"));
  Serial.print(F("[+] Trig Pin: D"));
  Serial.print(PIN_JSN_TRIG);
  Serial.print(F(", Echo Pin: D"));
  Serial.println(PIN_JSN_ECHO);
}

bool readJsnSr04tWaterLevel(uint16_t &waterLevelCm) {
  const uint8_t MAX_RETRIES = 3;

  for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++) {
    // 1. ส่งสัญญาณ Trigger Pulse (LOW 2us -> HIGH 10us -> LOW)
    digitalWrite(PIN_JSN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_JSN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_JSN_TRIG, LOW);

    // 2. วัดระยะเวลาที่สัญญาณ Echo เป็น HIGH (Timeout 35,000 us ~= 6 เมตร)
    unsigned long durationUs = pulseIn(PIN_JSN_ECHO, HIGH, 35000UL);

    if (durationUs > 0) {
      // ระยะทาง (cm) = (ระยะเวลา us * 0.0343) / 2
      // หรือ durationUs / 58.2
      float distance = (durationUs * 0.0343) / 2.0;

      // ตรวจสอบพิสัยของ JSN-SR04T (20cm - 600cm)
      if (distance >= 20.0 && distance <= 600.0) {
        waterLevelCm = (uint16_t)round(distance);

        Serial.print(F("[JSN-SR04T] OK: "));
        Serial.print(waterLevelCm);
        Serial.println(F("cm"));

        return true;
      }
    }

    // หากอ่านไม่สำเร็จ ให้หน่วง 60ms ก่อนลองใหม่ (เซนเซอร์ต้องใช้เวลาคลายคลื่นสะท้อน)
    if (attempt < MAX_RETRIES - 1) {
      delay(60);
    }
  }

  Serial.println(F("[JSN-SR04T] ERROR: Timeout or Out of Range"));
  return false;
}

#endif // USE_JSN_SR04T
