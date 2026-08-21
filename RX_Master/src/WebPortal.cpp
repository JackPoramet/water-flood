#include "WebPortal.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include "Config.h"
#include "SystemState.h"
#include "WebDashboard.h"
#include "TelegramNotifier.h"
#include "SettingsManager.h"

static WebServer server(80);

static void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

static void handleApiData() {
  if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    String json = "{";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"pollCycle\":" + String(pollCycleCount) + ",";
    json += "\"activeNode\":" + String(nodes[currentNodeIndex].id) + ",";
    json += "\"wifiSsid\":\"" + String(WiFi.SSID()) + "\",";
    json += "\"wifiIp\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"wifiRssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"telegramEnabled\":" + String(TELEGRAM_ENABLED ? "true" : "false") + ",";
    
    bool anyCritical = false;
    uint16_t maxLevel = 0;
    for (int i = 0; i < PROTOCOL_MAX_NODES; i++) {
      if (nodes[i].floodStatus == FLOOD_CRITICAL) anyCritical = true;
      if (nodes[i].waterLevelCm > maxLevel) maxLevel = nodes[i].waterLevelCm;
    }
    json += "\"floodAlert\":" + String(anyCritical ? "true" : "false") + ",";
    json += "\"maxLevelCm\":" + String(maxLevel) + ",";

    json += "\"nodes\":[";
    for (int i = 0; i < PROTOCOL_MAX_NODES; i++) {
      json += "{";
      json += "\"id\":" + String(nodes[i].id) + ",";
      json += "\"name\":\"" + String(nodes[i].name) + "\",";
      json += "\"online\":" + String(nodes[i].online ? "true" : "false") + ",";
      json += "\"waterLevelCm\":" + String(nodes[i].waterLevelCm) + ",";
      json += "\"waterPercent\":" + String(nodes[i].waterPercent) + ",";
      json += "\"floodStatus\":" + String(nodes[i].floodStatus) + ",";
      json += "\"warnThresholdCm\":" + String(nodes[i].warnThresholdCm) + ",";
      json += "\"critThresholdCm\":" + String(nodes[i].critThresholdCm) + ",";
      json += "\"batteryMv\":" + String(nodes[i].batteryMilliVolt) + ",";
      json += "\"uptimeSec\":" + String(nodes[i].uptimeSec) + ",";
      json += "\"rssi\":" + String(nodes[i].rssi) + ",";
      json += "\"snr\":" + String(nodes[i].snr, 1) + ",";
      unsigned long secAgo = nodes[i].lastSeenMillis > 0 ? (millis() - nodes[i].lastSeenMillis) / 1000 : 9999;
      json += "\"lastSeenSec\":" + String(secAgo) + ",";
      json += "\"packetsReceived\":" + String(nodes[i].packetsReceived);
      json += "}";
      if (i < PROTOCOL_MAX_NODES - 1) json += ",";
    }
    json += "]}";

    xSemaphoreGive(dataMutex);
    server.send(200, "application/json", json);
  } else {
    server.send(503, "application/json", "{\"error\":\"Resource busy\"}");
  }
}

static void handleApiPollNow() {
  triggerManualPoll = true;
  server.send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"Polling triggered\"}");
}

static void handleApiSaveSettings() {
  if (server.hasArg("nodeId") && server.hasArg("warnCm") && server.hasArg("critCm")) {
    uint8_t nodeId = server.arg("nodeId").toInt();
    uint16_t warnCm = server.arg("warnCm").toInt();
    uint16_t critCm = server.arg("critCm").toInt();

    int targetIdx = -1;
    for (int i = 0; i < PROTOCOL_MAX_NODES; i++) {
      if (nodes[i].id == nodeId) {
        targetIdx = i;
        break;
      }
    }

    if (targetIdx >= 0 && warnCm > 0 && critCm > warnCm) {
      if (saveNodeThresholds(targetIdx, warnCm, critCm)) {
        server.send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"บันทึกเกณฑ์ระดับน้ำสำเร็จ\"}");
        return;
      }
    }
  }
  server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"พารามิเตอร์ไม่ถูกต้อง (ต้องให้ วิกฤต > เฝ้าระวัง > 0)\"}");
}

static void handleApiTelegramTest() {
  if (!TELEGRAM_ENABLED) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Telegram is disabled in Config.h\"}");
    return;
  }
  if (strlen(TELEGRAM_BOT_TOKEN) == 0 || strlen(TELEGRAM_CHAT_ID) == 0) {
    server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Telegram Bot Token or Chat ID is not set!\"}");
    return;
  }

  String testMsg = "🧪 <b>[ทดสอบระบบแจ้งเตือน Telegram สำเร็จ]</b>\n";
  testMsg += "ESP32-S3 Master สามารถส่งข้อความเข้ากลุ่ม Telegram ได้อย่างสมบูรณ์!\n";
  testMsg += "⏱ <b>Uptime:</b> " + String(millis() / 1000) + " วินาที\n";
  testMsg += "📶 <b>WiFi:</b> " + String(WiFi.SSID()) + " (" + WiFi.localIP().toString() + ")";

  if (queueTelegramMessage(testMsg)) {
    server.send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"Telegram test notification queued!\"}");
  } else {
    server.send(500, "application/json", "{\"status\":\"error\",\"msg\":\"Failed to queue test message\"}");
  }
}

void TaskWebServer(void *pvParameters) {
  Serial.println(F("[FreeRTOS] Web Server Task started on Core 0"));

  // โหมด Station Mode (WIFI_STA): เชื่อมต่อ WiFi บ้าน/เราเตอร์อย่างเดียว
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart < 10000)) {
    vTaskDelay(pdMS_TO_TICKS(300));
  }

  // เริ่มต้น Web Server ก่อน
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/data", HTTP_GET, handleApiData);
  server.on("/api/poll", HTTP_POST, handleApiPollNow);
  server.on("/api/settings", HTTP_POST, handleApiSaveSettings);
  server.on("/api/telegram/test", HTTP_POST, handleApiTelegramTest);
  server.on("/api/telegram/test", HTTP_GET, handleApiTelegramTest);
  server.begin();
  Serial.println(F("[HTTP] Web Server Started on Port 80!"));

  // เริ่มต้น mDNS หลังจาก Server พร้อม
  if (MDNS.begin("floodmonitor")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println(F("[mDNS] Responder active: http://floodmonitor.local"));
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("=================================================="));
    Serial.printf ("[WiFi Station] Connected! Open Dashboard at:\n");
    Serial.printf ("   -> http://%s\n", WiFi.localIP().toString().c_str());
    Serial.printf ("   -> http://floodmonitor.local\n");
    Serial.println(F("=================================================="));

    // ส่งข้อความแจ้งเตือนเมื่อระบบเริ่มทำงาน (Boot Alert)
    if (TELEGRAM_ALERT_BOOT && TELEGRAM_ENABLED) {
      String bootMsg = "🌊 <b>[ระบบตรวจเช็คน้ำท่วม Master เริ่มต้นสำเร็จ]</b>\n";
      bootMsg += "📡 <b>IP Address:</b> " + WiFi.localIP().toString() + "\n";
      bootMsg += "📶 <b>WiFi:</b> " + String(WiFi.SSID()) + " (" + String(WiFi.RSSI()) + " dBm)\n";
      bootMsg += "🚀 <b>สถานะ:</b> ระบบพร้อมรับสัญญาณ LoRa 2 โหนด";
      queueTelegramMessage(bootMsg);
    }
  } else {
    Serial.println(F("[WiFi Station] Timeout! Cannot connect to WiFi."));
  }

  unsigned long lastReconnectCheck = 0;

  for (;;) {
    server.handleClient();

    if (millis() - lastReconnectCheck >= 10000) {
      lastReconnectCheck = millis();
      if (WiFi.status() != WL_CONNECTED) {
        WiFi.reconnect();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
