#ifndef MODBUS_MANAGER_H
#define MODBUS_MANAGER_H

#include <Arduino.h>
#include "Config.h"

#ifdef USE_MODBUS_SENSOR

// =========================================================================
// โมดูลจัดการ RS485 Modbus RTU สำหรับเซนเซอร์ DJLK-003AB
// รับผิดชอบ: Serial1 (Hardware UART), MAX485 DE/RE, ModbusMaster Library
// =========================================================================

// เริ่มต้น Serial1, MAX485 pins, และ ModbusMaster
void initModbus();

// อ่านค่าระดับน้ำจาก DJLK-003AB (Register 0x0100)
// คืนค่า true หากอ่านสำเร็จ, waterLevelMm จะได้ค่าระยะทาง (mm)
bool readModbusWaterLevel(uint16_t &waterLevelMm);

// ตรวจสอบสถานะเซนเซอร์ (ออนไลน์/ออฟไลน์)
bool isModbusSensorOnline();

#endif // USE_MODBUS_SENSOR

#endif // MODBUS_MANAGER_H
