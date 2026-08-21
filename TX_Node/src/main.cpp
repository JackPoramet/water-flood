/*
 * JSN-SR04T Full Diagnostic
 * ทดสอบทุกรูปแบบเพื่อหาสาเหตุที่เซนเซอร์ไม่ตอบ
 */

#include <Arduino.h>

#define Trig_PIN 9
#define Echo_PIN 12

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) { delay(10); }

  Serial.println(F("=== JSN-SR04T Full Diagnostic ==="));
  Serial.print(F("Trig: D")); Serial.print(Trig_PIN);
  Serial.print(F(", Echo: D")); Serial.println(Echo_PIN);
  Serial.println();

  pinMode(Trig_PIN, OUTPUT);
  pinMode(Echo_PIN, INPUT);
  digitalWrite(Trig_PIN, LOW);
}

// ทดสอบ 1: ตรวจสถานะ Echo pin ดิบๆ
void testRawEcho() {
  Serial.print(F("[RAW] Echo pin = "));
  Serial.println(digitalRead(Echo_PIN) ? "HIGH" : "LOW");
}

// ทดสอบ 2: Trigger แบบพัลส์ยาว (20us, 50us, 100us)
void testPulse(unsigned int trigUs) {
  digitalWrite(Trig_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(Trig_PIN, HIGH);
  delayMicroseconds(trigUs);
  digitalWrite(Trig_PIN, LOW);
  
  unsigned long pw = pulseIn(Echo_PIN, HIGH, 60000UL);
  
  Serial.print(F("[PULSE "));
  Serial.print(trigUs);
  Serial.print(F("us] PW="));
  Serial.print(pw);
  if (pw > 0) {
    unsigned int dist = pw * 0.0173681;
    Serial.print(F(" us -> "));
    Serial.print(dist);
    Serial.println(F(" cm  <<<< OK!"));
  } else {
    Serial.println(F(" us -> No echo"));
  }
}

// ทดสอบ 3: ตรวจว่า Echo ยก HIGH ขึ้นหรือเปล่า (Manual poll)
void testManualPoll() {
  digitalWrite(Trig_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(Trig_PIN, HIGH);
  delayMicroseconds(20);
  digitalWrite(Trig_PIN, LOW);
  
  // วนอ่าน Echo ด้วยมือ 5000 รอบ
  unsigned int highCount = 0;
  for (unsigned int i = 0; i < 5000; i++) {
    if (digitalRead(Echo_PIN) == HIGH) {
      highCount++;
    }
    delayMicroseconds(10);
  }
  Serial.print(F("[MANUAL] Echo HIGH count = "));
  Serial.print(highCount);
  Serial.print(F(" / 5000"));
  if (highCount > 0) {
    Serial.println(F("  <<<< DETECTED!"));
  } else {
    Serial.println(F("  (Echo never went HIGH)"));
  }
}

// ทดสอบ 4: Trig ค้าง HIGH แล้วดูว่า Echo เปลี่ยนไหม
void testTrigHold() {
  Serial.print(F("[HOLD] Echo before Trig HIGH = "));
  Serial.print(digitalRead(Echo_PIN) ? "HIGH" : "LOW");
  
  digitalWrite(Trig_PIN, HIGH);
  delay(1);
  
  Serial.print(F(", after Trig HIGH 1ms = "));
  Serial.print(digitalRead(Echo_PIN) ? "HIGH" : "LOW");
  
  digitalWrite(Trig_PIN, LOW);
  delay(1);
  
  Serial.print(F(", after Trig LOW = "));
  Serial.println(digitalRead(Echo_PIN) ? "HIGH" : "LOW");
}

void loop() {
  Serial.println(F("--- Cycle ---"));
  
  testRawEcho();
  delay(100);
  
  testPulse(10);    // มาตรฐาน 10us
  delay(100);
  
  testPulse(50);    // ยาวขึ้น 50us
  delay(100);
  
  testPulse(100);   // ยาวมาก 100us
  delay(100);
  
  testManualPoll(); // Manual poll หา Echo HIGH
  delay(100);
  
  testTrigHold();   // ค้าง Trig HIGH
  
  Serial.println();
  delay(2000);
}