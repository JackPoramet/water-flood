#ifndef HC_SR04_MANAGER_H
#define HC_SR04_MANAGER_H

#include <Arduino.h>
#include "Config.h"

#ifdef USE_HC_SR04

// =========================================================================
// โมดูลจัดการเซนเซอร์อัลตร้าโซนิค HC-SR04 (Node 3)
// รับผิดชอบ: ควบคุมสัญญาณ Trigger/Echo, pulseIn, คำนวณระยะทาง (cm)
// =========================================================================

// เริ่มต้นขา Trigger (D12) และ Echo (D13)
void initHcSr04();

// ส่งคลื่น Ultrasonic Ping และอ่านระยะทาง (cm)
// คืนค่า true หากอ่านสำเร็จ (ระยะอยู่ในช่วง 2 - 400 cm), false หาก timeout หรือหลุดช่วง
bool readHcSr04WaterLevel(uint16_t &waterLevelCm);

#endif // USE_HC_SR04

#endif // HC_SR04_MANAGER_H
