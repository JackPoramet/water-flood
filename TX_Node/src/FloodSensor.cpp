#include "FloodSensor.h"

#if defined(USE_MODBUS_SENSOR)
// =========================================================================
// Node 1: อ่านค่าจริงจากเซนเซอร์ DJLK-003AB ผ่าน ModbusManager
// =========================================================================
#include "ModbusManager.h"

static uint16_t lastWaterLevelMm = 0;

void initFloodSensor() {
  initModbus();
}

bool readFloodSensor(uint8_t nodeId, unsigned long bootMillis, SensorPayload &payload) {
  uint16_t waterLevelMm = 0;
  bool success = readModbusWaterLevel(waterLevelMm);

  if (success) {
    lastWaterLevelMm = waterLevelMm;
  }
  uint16_t useMm = (lastWaterLevelMm > 0) ? lastWaterLevelMm : waterLevelMm;

  payload.waterLevelCm = useMm / 10;
  payload.waterPercent = constrain(map(payload.waterLevelCm, 0, 300, 0, 100), 0, 100);

  if (payload.waterLevelCm >= 200) {
    payload.floodStatus = FLOOD_CRITICAL;
  } else if (payload.waterLevelCm >= 100) {
    payload.floodStatus = FLOOD_WARNING;
  } else {
    payload.floodStatus = FLOOD_NORMAL;
  }

  payload.batteryMilliVolt = 0;
  payload.uptimeSec = (uint16_t)((millis() - bootMillis) / 1000);

  return success;
}

#elif defined(USE_JSN_SR04T)
// =========================================================================
// Node 2: อ่านค่าจริงจากเซนเซอร์ JSN-SR04T ผ่าน JsnSr04tManager
// =========================================================================
#include "JsnSr04tManager.h"

static uint16_t lastWaterLevelCm = 0;

void initFloodSensor() {
  initJsnSr04t();
}

bool readFloodSensor(uint8_t nodeId, unsigned long bootMillis, SensorPayload &payload) {
  uint16_t measuredCm = 0;
  bool success = readJsnSr04tWaterLevel(measuredCm);

  if (success) {
    lastWaterLevelCm = measuredCm;
  }
  uint16_t useCm = (lastWaterLevelCm > 0) ? lastWaterLevelCm : measuredCm;

  payload.waterLevelCm = useCm;
  payload.waterPercent = constrain(map(payload.waterLevelCm, 0, 300, 0, 100), 0, 100);

  if (payload.waterLevelCm >= 200) {
    payload.floodStatus = FLOOD_CRITICAL;
  } else if (payload.waterLevelCm >= 100) {
    payload.floodStatus = FLOOD_WARNING;
  } else {
    payload.floodStatus = FLOOD_NORMAL;
  }

  payload.batteryMilliVolt = 0;
  payload.uptimeSec = (uint16_t)((millis() - bootMillis) / 1000);

  return success;
}

#elif defined(USE_HC_SR04)
// =========================================================================
// Node 3: อ่านค่าจริงจากเซนเซอร์ HC-SR04 ผ่าน HcSr04Manager
// =========================================================================
#include "HcSr04Manager.h"

static uint16_t lastWaterLevelCm = 0;

void initFloodSensor() {
  initHcSr04();
}

bool readFloodSensor(uint8_t nodeId, unsigned long bootMillis, SensorPayload &payload) {
  uint16_t measuredCm = 0;
  bool success = readHcSr04WaterLevel(measuredCm);

  if (success) {
    lastWaterLevelCm = measuredCm;
  }
  uint16_t useCm = (lastWaterLevelCm > 0) ? lastWaterLevelCm : measuredCm;

  payload.waterLevelCm = useCm;
  payload.waterPercent = constrain(map(payload.waterLevelCm, 0, 300, 0, 100), 0, 100);

  if (payload.waterLevelCm >= 200) {
    payload.floodStatus = FLOOD_CRITICAL;
  } else if (payload.waterLevelCm >= 100) {
    payload.floodStatus = FLOOD_WARNING;
  } else {
    payload.floodStatus = FLOOD_NORMAL;
  }

  payload.batteryMilliVolt = 0;
  payload.uptimeSec = (uint16_t)((millis() - bootMillis) / 1000);

  return success;
}

#else
// =========================================================================
// Fallback: เมื่อไม่มีการกำหนดชนิดเซนเซอร์
// =========================================================================
void initFloodSensor() {}

bool readFloodSensor(uint8_t nodeId, unsigned long bootMillis, SensorPayload &payload) {
  payload.waterLevelCm = 0;
  payload.waterPercent = 0;
  payload.floodStatus = FLOOD_NORMAL;
  payload.batteryMilliVolt = 0;
  payload.uptimeSec = (uint16_t)((millis() - bootMillis) / 1000);
  return false;
}
#endif
