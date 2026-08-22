#include "JsnSr04tManager.h"

#ifdef USE_JSN_SR04T

// =========================================================================
// JsnSr04tManager — จัดการเซนเซอร์วัดระยะอัลตร้าโซนิคกันน้ำ JSN-SR04T (Node 2)
// =========================================================================
// ขาต่อใช้งานบน LoRa32u4:
//   Trig Pin : A3 (Output) -> ส่งพัลส์ 20us
//   Echo Pin : A4 (Input)  -> รับพัลส์สะท้อนกลับ (Timeout 45,000us ~= 7.5 เมตร)
//   VCC      : 5V (USB Pin)
//   GND      : GND
// =========================================================================

void initJsnSr04t() {
  pinMode(PIN_JSN_TRIG, OUTPUT);
  digitalWrite(PIN_JSN_TRIG, LOW);
  pinMode(PIN_JSN_ECHO, INPUT);

  Serial.println(F("[+] JSN-SR04T Waterproof Ultrasonic Initialized (5V Pull-up Active)"));
  Serial.println(F("    Trig Pin: A3, Echo Pin: A4"));
}

bool readJsnSr04tWaterLevel(uint16_t &waterLevelCm) {
  // 1. ดึงขา Trig ลง LOW (0V) เพื่อเตรียมพร้อม
  pinMode(PIN_JSN_TRIG, OUTPUT);
  digitalWrite(PIN_JSN_TRIG, LOW);
  delayMicroseconds(5);

  // 2. ปล่อยขา Trig เป็น INPUT (Hi-Z) เพื่อให้ R 10k ดึงสัญญาณขึ้น 5.0V เต็ม
  pinMode(PIN_JSN_TRIG, INPUT);
  delayMicroseconds(15);

  // 3. ดึงกลับลง LOW (0V) เพื่อจบสัญญาณ Trigger Pulse
  pinMode(PIN_JSN_TRIG, OUTPUT);
  digitalWrite(PIN_JSN_TRIG, LOW);

  // 4. อ่านค่าความกว้างสัญญาณ Echo (Timeout 60,000 us ~= 10 เมตร)
  long duration = pulseIn(PIN_JSN_ECHO, HIGH, 60000UL);

  // 5. คำนวณระยะทางตามสูตรเสียง: distance = duration * 0.034 / 2
  int distance = (int)(duration * 0.034 / 2);

  // แสดงผลบน Serial Monitor
  Serial.print(F("Distance = "));
  Serial.print(distance);
  Serial.print(F(" cm (Pulse: "));
  Serial.print(duration);
  Serial.println(F(" us)"));

  if (distance >= 18 && distance <= 600) {
    waterLevelCm = (uint16_t)distance;
    return true;
  }

  return false;
}

#endif // USE_JSN_SR04T
