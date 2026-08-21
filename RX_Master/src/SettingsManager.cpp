#include "SettingsManager.h"
#include <Preferences.h>
#include "Config.h"
#include "SystemState.h"

static Preferences prefs;

void initSettings() {
  loadSettings();
}

void loadSettings() {
  if (prefs.begin("flood_cfg", false)) {
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      for (int i = 0; i < PROTOCOL_MAX_NODES; i++) {
        String keyWarn = "w_" + String(nodes[i].id);
        String keyCrit = "c_" + String(nodes[i].id);

        uint16_t warn = prefs.getUShort(keyWarn.c_str(), DEFAULT_WARN_THRESHOLD_CM);
        uint16_t crit = prefs.getUShort(keyCrit.c_str(), DEFAULT_CRIT_THRESHOLD_CM);

        if (warn == 0) warn = DEFAULT_WARN_THRESHOLD_CM;
        if (crit == 0) crit = DEFAULT_CRIT_THRESHOLD_CM;

        nodes[i].warnThresholdCm = warn;
        nodes[i].critThresholdCm = crit;

        Serial.printf("[NVS] Loaded Node #%d Thresholds: Warn=%d cm, Crit=%d cm\n",
                      nodes[i].id, warn, crit);
      }
      xSemaphoreGive(dataMutex);
    }
    prefs.end();
  } else {
    Serial.println(F("[-] Failed to open Preferences NVS! Using defaults."));
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      for (int i = 0; i < PROTOCOL_MAX_NODES; i++) {
        nodes[i].warnThresholdCm = DEFAULT_WARN_THRESHOLD_CM;
        nodes[i].critThresholdCm = DEFAULT_CRIT_THRESHOLD_CM;
      }
      xSemaphoreGive(dataMutex);
    }
  }
}

bool saveNodeThresholds(uint8_t nodeIndex, uint16_t warnCm, uint16_t critCm) {
  if (nodeIndex >= PROTOCOL_MAX_NODES) return false;
  if (warnCm == 0 || critCm == 0) return false;

  if (prefs.begin("flood_cfg", false)) {
    uint8_t nodeId = 0;

    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      nodeId = nodes[nodeIndex].id;
      nodes[nodeIndex].warnThresholdCm = warnCm;
      nodes[nodeIndex].critThresholdCm = critCm;
      xSemaphoreGive(dataMutex);
    }

    String keyWarn = "w_" + String(nodeId);
    String keyCrit = "c_" + String(nodeId);

    prefs.putUShort(keyWarn.c_str(), warnCm);
    prefs.putUShort(keyCrit.c_str(), critCm);
    prefs.end();

    Serial.printf("[NVS] Saved Node #%d Thresholds to NVS: Warn=%d cm, Crit=%d cm\n",
                  nodeId, warnCm, critCm);
    return true;
  } else {
    Serial.println(F("[-] Failed to open NVS for writing!"));
    return false;
  }
}
