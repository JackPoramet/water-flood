#ifndef FLOOD_SENSOR_H
#define FLOOD_SENSOR_H

#include <Arduino.h>
#include "Protocol.h"

// ฟังก์ชันสร้างข้อมูลจำลองเซนเซอร์วัดระดับน้ำ
void generateFloodData(uint8_t nodeId, unsigned long bootMillis, SensorPayload &payload);

#endif // FLOOD_SENSOR_H
