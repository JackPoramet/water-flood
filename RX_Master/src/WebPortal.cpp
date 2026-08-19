#include "WebPortal.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include "Config.h"
#include "SystemState.h"
#include "WebDashboard.h"

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
  } else {
    Serial.println(F("[WiFi Station] Timeout! You can connect to 'FloodMonitor-AP' at http://192.168.4.1"));
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
