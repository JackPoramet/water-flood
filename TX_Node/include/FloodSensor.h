#ifndef FLOOD_SENSOR_H
#define FLOOD_SENSOR_H

#include <Arduino.h>
#include "Protocol.h"
#include "Config.h"

// =========================================================================
// โมดูลเซนเซอร์วัดระดับน้ำ — Unified Interface สำหรับทุก Node
// =========================================================================

#if defined(USE_MODBUS_SENSOR) || defined(USE_JSN_SR04T) || defined(USE_HC_SR04)
// เริ่มต้นฮาร์ดแวร์เซนเซอร์จริง (Node 1: DJLK-003AB, Node 2: JSN-SR04T, Node 3: HC-SR04)
void initFloodSensor();
#endif

// อ่านค่าเซนเซอร์แล้วบรรจุลง SensorPayload
// - Node 1 (USE_MODBUS_SENSOR): อ่านค่าจริงจาก DJLK-003AB (ModbusManager)
// - Node 2 (USE_JSN_SR04T): อ่านค่าจริงจาก JSN-SR04T (JsnSr04tManager)
// - Node 3 (USE_HC_SR04): อ่านค่าจริงจาก HC-SR04 (HcSr04Manager)
bool readFloodSensor(uint8_t nodeId, unsigned long bootMillis, SensorPayload &payload);

#endif // FLOOD_SENSOR_H
