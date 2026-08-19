#include "FloodSensor.h"

void generateFloodData(uint8_t nodeId, unsigned long bootMillis, SensorPayload &payload) {
  unsigned long now = millis();
  float t = (now - bootMillis) / 1000.0;

  if (nodeId == NODE_ID_1) {
    // Node 1: ระดับน้ำปกติ (35 - 75 cm)
    payload.waterLevelCm = (uint16_t)(55.0 + 20.0 * sin(t / 25.0));
  } else if (nodeId == NODE_ID_2) {
    // Node 2: เฝ้าระวัง (120 - 170 cm)
    payload.waterLevelCm = (uint16_t)(145.0 + 25.0 * sin(t / 18.0));
  } else {
    // Node 3: วิกฤตน้ำท่วม (215 - 280 cm)
    payload.waterLevelCm = (uint16_t)(245.0 + 35.0 * sin(t / 12.0));
  }

  payload.waterPercent = constrain(map(payload.waterLevelCm, 0, 300, 0, 100), 0, 100);

  if (payload.waterLevelCm >= 200) {
    payload.floodStatus = FLOOD_CRITICAL;
  } else if (payload.waterLevelCm >= 100) {
    payload.floodStatus = FLOOD_WARNING;
  } else {
    payload.floodStatus = FLOOD_NORMAL;
  }

  payload.batteryMilliVolt = (uint16_t)(4050 - (t / 120.0));
  if (payload.batteryMilliVolt < 3500) payload.batteryMilliVolt = 3500;

  payload.uptimeSec = (uint16_t)((now - bootMillis) / 1000);
}
