#ifndef JSN_SR04T_MANAGER_H
#define JSN_SR04T_MANAGER_H

#include <Arduino.h>
#include "Config.h"

#ifdef USE_JSN_SR04T

// =========================================================================
// โมดูลจัดการเซนเซอร์อัลตร้าโซนิคกันน้ำ JSN-SR04T (Node 2)
// รับผิดชอบ: ควบคุมสัญญาณ Trigger/Echo, pulseIn, คำนวณระยะทาง (cm)
// =========================================================================

// เริ่มต้นขา Trigger และ Echo
void initJsnSr04t();

// ส่งคลื่น Ultrasonic Ping และอ่านระยะทาง (cm)
// คืนค่า true หากอ่านสำเร็จ (ระยะอยู่ในช่วง 20 - 600 cm), false หาก timeout หรือหลุดช่วง
bool readJsnSr04tWaterLevel(uint16_t &waterLevelCm);

#endif // USE_JSN_SR04T

#endif // JSN_SR04T_MANAGER_H
