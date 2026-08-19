#include <Arduino.h>
#include "Config.h"
#include "SystemState.h"
#include "WebPortal.h"
#include "LoRaEngine.h"
#include "DisplayManager.h"

// =========================================================================
// 1. การตั้งค่าข้อมูล WiFi และตัวแปรระบบส่วนกลาง
// =========================================================================
const char* WIFI_SSID = "CoE#01";     // ชื่อ WiFi ของท่าน
const char* WIFI_PASS = "xxxxxxxx"; // รหัสผ่าน WiFi

SemaphoreHandle_t dataMutex = NULL;

NodeInfo nodes[PROTOCOL_MAX_NODES] = {
  {NODE_ID_1, "จุดที่ 1 (คลองระบายน้ำ)", false, 0, 0, FLOOD_NORMAL, 0, 0, 0, 0.0, 0, 0, 0},
  {NODE_ID_2, "จุดที่ 2 (ริมแม่น้ำเฝ้าระวัง)", false, 0, 0, FLOOD_NORMAL, 0, 0, 0, 0.0, 0, 0, 0},
  {NODE_ID_3, "จุดที่ 3 (จุดเสี่ยงน้ำท่วมชุมชน)", false, 0, 0, FLOOD_NORMAL, 0, 0, 0, 0.0, 0, 0, 0}
};

PollState currentPollState = STATE_SEND_POLL;
uint8_t currentNodeIndex = 0;
uint32_t pollCycleCount = 0;
uint8_t animStep = 0;
bool triggerManualPoll = false;

// =========================================================================
// 2. Setup & Main Entry Point
// =========================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println(F("\n=================================================="));
  Serial.println(F(" ESP32-S3 Dual-Core FreeRTOS Flood Master System "));
  Serial.println(F("=================================================="));

  // สร้าง Mutex ป้องกันข้อมูลซ้อนทับ
  dataMutex = xSemaphoreCreateMutex();

  // Task 1 (Core 0): WiFi, mDNS, WebServer & REST API
  xTaskCreatePinnedToCore(
    TaskWebServer,
    "TaskWebServer",
    8192,
    NULL,
    1,
    NULL,
    0 // Core 0
  );

  // Task 2 (Core 1): LoRa SX1278 Polling Engine
  xTaskCreatePinnedToCore(
    TaskLoRaPolling,
    "TaskLoRaPolling",
    8192,
    NULL,
    2,
    NULL,
    1 // Core 1
  );

  // Task 3 (Core 1): OLED Display UI
  xTaskCreatePinnedToCore(
    TaskOLEDDisplay,
    "TaskOLEDDisplay",
    4096,
    NULL,
    1,
    NULL,
    1 // Core 1
  );

  Serial.println(F("[+] FreeRTOS Multi-Core Tasks Spawned Successfully!"));
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
