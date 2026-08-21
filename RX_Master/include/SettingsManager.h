#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>

// ฟังก์ชันเริ่มต้นและโหลดค่าการตั้งค่าจาก Flash NVS / EEPROM
void initSettings();
void loadSettings();

// ฟังก์ชันบันทึกค่าระดับน้ำเตือนภัยของโหนดลง NVS
bool saveNodeThresholds(uint8_t nodeIndex, uint16_t warnCm, uint16_t critCm);

#endif // SETTINGS_MANAGER_H
