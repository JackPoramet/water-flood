#include <Arduino.h>
#include "Config.h"
#include "SystemState.h"
#include "WebPortal.h"
#include "LoRaEngine.h"
#include "DisplayManager.h"
#include "TelegramNotifier.h"
#include "SettingsManager.h"

// =========================================================================
// 1. การตั้งค่าข้อมูล WiFi, Telegram และตัวแปรระบบส่วนกลาง
// =========================================================================
const char* WIFI_SSID = "SnackJack";     // ชื่อ WiFi ของท่าน
const char* WIFI_PASS = "xxxxxxxx"; // รหัสผ่าน WiFi

// --- การตั้งค่า Telegram Bot & Group Chat ID ---
// รับ Token ได้จาก @BotFather และ Chat ID กลุ่มได้จาก @userinfobot หรือ @raw_data_bot
const char* TELEGRAM_BOT_TOKEN = "8850291145:AAEhqiyjg8JvGca5jmiRVgu4NZjWyG_HgYo";
const char* TELEGRAM_CHAT_ID   = "-1004431395744";

SemaphoreHandle_t dataMutex = NULL;

NodeInfo nodes[PROTOCOL_MAX_NODES] = {
  {NODE_ID_1, "Node 1", false, 0, 0, FLOOD_NORMAL, 0, 0, 0, 0.0, 0, 0, 0, FLOOD_NORMAL, false, 0, DEFAULT_WARN_THRESHOLD_CM, DEFAULT_CRIT_THRESHOLD_CM},
  {NODE_ID_2, "Node 2", false, 0, 0, FLOOD_NORMAL, 0, 0, 0, 0.0, 0, 0, 0, FLOOD_NORMAL, false, 0, DEFAULT_WARN_THRESHOLD_CM, DEFAULT_CRIT_THRESHOLD_CM}
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

  // โหลดค่าเกณฑ์ระดับน้ำจาก Flash NVS / EEPROM
  initSettings();

  // เริ่มต้นคิวสำหรับระบบแจ้งเตือน Telegram
  initTelegramNotifier();

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

  // Task 4 (Core 0): Telegram Notification & Interactive Bot Engine (HTTPS TLS)
  // Priority 2: สูงกว่า WebServer เพื่อให้ส่งแจ้งเตือนเร่งด่วน (Critical/Warning) ได้ทันที
  xTaskCreatePinnedToCore(
    TaskTelegram,
    "TaskTelegram",
    10240,
    NULL,
    2,
    NULL,
    0 // Core 0
  );

  Serial.println(F("[+] FreeRTOS Multi-Core Tasks Spawned Successfully!"));
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
