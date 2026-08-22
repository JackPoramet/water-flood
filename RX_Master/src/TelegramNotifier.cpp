#include "TelegramNotifier.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "SystemState.h"

static QueueHandle_t telegramQueue = NULL;
static int64_t lastTelegramUpdateId = 0;
static bool isFirstUpdateCheck = true;

static String jsonEscape(const String& s) {
  String out = "";
  out.reserve(s.length() + 32);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') continue;
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

void initTelegramNotifier() {
  if (telegramQueue == NULL) {
    telegramQueue = xQueueCreate(10, sizeof(TelegramMessage));
    if (telegramQueue != NULL) {
      Serial.println(F("[Telegram] Queue initialized (Depth: 10)"));
    } else {
      Serial.println(F("[-] Failed to create Telegram Queue!"));
    }
  }
}

bool queueTelegramMessage(const String& text, const String& targetChatId, uint8_t priority) {
  if (!TELEGRAM_ENABLED) return false;
  if (strlen(TELEGRAM_BOT_TOKEN) == 0 || strlen(TELEGRAM_CHAT_ID) == 0) {
    Serial.println(F("[Telegram] Bot Token or Chat ID not configured. Skip sending."));
    return false;
  }
  if (telegramQueue == NULL) {
    initTelegramNotifier();
  }

  TelegramMessage msg;
  memset(&msg, 0, sizeof(TelegramMessage));
  strncpy(msg.text, text.c_str(), TELEGRAM_MAX_MSG_LEN - 1);
  if (targetChatId.length() > 0) {
    strncpy(msg.chatId, targetChatId.c_str(), TELEGRAM_MAX_CHAT_LEN - 1);
  } else {
    strncpy(msg.chatId, TELEGRAM_CHAT_ID, TELEGRAM_MAX_CHAT_LEN - 1);
  }
  msg.priority = priority;

  BaseType_t result;
  if (priority >= TELE_PRIORITY_URGENT) {
    // ข้อความเร่งด่วน (Critical/Warning/Offline): แทรกหัวคิว ส่งก่อนข้อความอื่น
    result = xQueueSendToFront(telegramQueue, &msg, 0);
    Serial.println(F("[Telegram] URGENT message enqueued to FRONT of queue!"));
  } else {
    // ข้อความปกติ: ต่อท้ายคิวตามลำดับ
    result = xQueueSendToBack(telegramQueue, &msg, 0);
    Serial.println(F("[Telegram] Message enqueued to back of queue."));
  }

  if (result != pdPASS) {
    Serial.println(F("[Telegram] Queue is full! Dropping message."));
    return false;
  }
  return true;
}

bool sendTelegramDirect(const String& text, const String& targetChatId) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[Telegram] WiFi not connected. Cannot send message."));
    return false;
  }

  if (strlen(TELEGRAM_BOT_TOKEN) == 0) {
    Serial.println(F("[Telegram] Token empty."));
    return false;
  }

  String destChatId = (targetChatId.length() > 0) ? targetChatId : String(TELEGRAM_CHAT_ID);
  if (destChatId.length() == 0) {
    Serial.println(F("[Telegram] Chat ID empty."));
    return false;
  }

  const int maxAttempts = 2;
  for (int attempt = 1; attempt <= maxAttempts; attempt++) {
    IPAddress telegramIP;
    bool dnsOk = WiFi.hostByName("api.telegram.org", telegramIP);
    
    if (!dnsOk) {
      Serial.printf("[Telegram] DNS lookup failed (Attempt %d/%d). Applying Public DNS (8.8.8.8, 1.1.1.1)...\n", attempt, maxAttempts);
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, IPAddress(8, 8, 8, 8), IPAddress(1, 1, 1, 1));
      vTaskDelay(pdMS_TO_TICKS(500));
      dnsOk = WiFi.hostByName("api.telegram.org", telegramIP);
      if (!dnsOk) {
        if (attempt == maxAttempts) {
          Serial.println(F("[-] [Telegram] Error: DNS Resolution failed after all retries."));
          return false;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        continue;
      }
    }

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(6000);
    client.setHandshakeTimeout(6);

    HTTPClient https;
    https.setTimeout(6000);
    https.setReuse(false);

    String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage";
    if (https.begin(client, url)) {
      https.addHeader("Content-Type", "application/json; charset=utf-8");

      String payload = "{\"chat_id\":\"" + destChatId + "\",\"text\":\"" + jsonEscape(text) + "\",\"parse_mode\":\"HTML\"}";

      int httpCode = https.POST(payload);
      https.end();
      client.stop(); // ปิด Socket และคืนหน่วยความจำทันที ป้องกัน Socket Exhaustion

      if (httpCode == HTTP_CODE_OK || httpCode == 200) {
        Serial.println(F("[Telegram] Message sent to Telegram successfully!"));
        return true;
      } else {
        Serial.printf("[Telegram] HTTP POST attempt %d failed, code: %d\n", attempt, httpCode);
        if (attempt < maxAttempts) {
          vTaskDelay(pdMS_TO_TICKS(1000));
        }
      }
    } else {
      https.end();
      client.stop();
      Serial.printf("[Telegram] Failed to begin HTTPS connection (Attempt %d/%d)\n", attempt, maxAttempts);
      if (attempt < maxAttempts) {
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
    }
  }

  return false;
}

// =========================================================================
// ตัวประมวลผลคำสั่ง Telegram Bot (Interactive Command Processor)
// =========================================================================
void processTelegramCommand(String cmd, const String& chatId, const String& fromUser) {
  cmd.trim();
  // ตัดชื่อบอทที่ต่อท้ายคำสั่งในกลุ่ม เช่น "/status@MyBot" -> "/status"
  int atIdx = cmd.indexOf('@');
  if (atIdx > 0) {
    cmd = cmd.substring(0, atIdx);
  }
  // แปลงเป็นตัวพิมพ์เล็กเพื่อความยืดหยุ่น
  cmd.toLowerCase();

  Serial.printf("[Telegram Bot] Received Command '%s' from Chat ID '%s' (User: %s)\n", cmd.c_str(), chatId.c_str(), fromUser.c_str());

  // --- คำสั่ง /status หรือ /data ---
  if (cmd == "/status" || cmd == "/data" || cmd == "/stat") {
    NodeInfo copyNodes[PROTOCOL_MAX_NODES];
    uint32_t cycle = 0;
    bool lockOk = false;

    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
      memcpy(copyNodes, nodes, sizeof(copyNodes));
      cycle = pollCycleCount;
      lockOk = true;
      xSemaphoreGive(dataMutex);
    }

    if (!lockOk) {
      sendTelegramDirect("⚠️ ระบบกำลังประมวลผลข้อมูล กรุณาลองใหม่อีกครั้งใน 2 วินาที", chatId);
      return;
    }

    String resp = "📊 <b>[รายงานสถานะระดับน้ำท่วมปัจจุบัน]</b> 📊\n";
    resp += "⏱ <b>Uptime:</b> " + String(millis() / 1000) + " วิ | <b>รอบ Polling:</b> " + String(cycle) + "\n";
    resp += "📶 <b>WiFi:</b> " + String(WiFi.SSID()) + " (" + String(WiFi.RSSI()) + " dBm)\n";
    resp += "🌐 <b>Dashboard:</b> http://" + WiFi.localIP().toString() + "\n";
    resp += "━━━━━━━━━━━━━━━━━━━\n";

    for (int i = 0; i < PROTOCOL_MAX_NODES; i++) {
      resp += (copyNodes[i].online ? "🟢" : "🔴");
      resp += " <b>Node #" + String(copyNodes[i].id) + " - " + String(copyNodes[i].name) + "</b>\n";
      
      if (copyNodes[i].online) {
        String floodBadge = "🟢 ปลอดภัย (SAFE)";
        if (copyNodes[i].floodStatus == FLOOD_CRITICAL) floodBadge = "🚨 วิกฤตน้ำท่วม (CRITICAL)";
        else if (copyNodes[i].floodStatus == FLOOD_WARNING) floodBadge = "⚠️ เฝ้าระวัง (WARNING)";

        resp += " • <b>ระยะห่างผิวน้ำ:</b> <b>" + String(copyNodes[i].waterLevelCm) + " cm</b> (" + String(copyNodes[i].waterPercent) + "%)\n";
        resp += " • <b>สถานะเตือนภัย:</b> " + floodBadge + "\n";
        resp += " • <b>คุณภาพสัญญาณ:</b> RSSI " + String(copyNodes[i].rssi) + " dBm | SNR " + String(copyNodes[i].snr, 1) + " dB\n";
        
        unsigned long secAgo = (copyNodes[i].lastSeenMillis > 0) ? (millis() - copyNodes[i].lastSeenMillis) / 1000 : 0;
        resp += " • <b>อัปเดตล่าสุด:</b> " + String(secAgo) + " วินาทีที่แล้ว\n";
      } else {
        resp += " • <b>สถานะ:</b> ❌ ขาดการเชื่อมต่อ (OFFLINE)\n";
        resp += " • <b>รายละเอียด:</b> ไม่ตอบสนองสัญญาณ LoRa\n";
      }
      if (i < PROTOCOL_MAX_NODES - 1) resp += "┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈┈\n";
    }

    resp += "━━━━━━━━━━━━━━━━━━━\n";
    resp += "💡 <i>พิมพ์ /help เพื่อดูคำสั่งทั้งหมด</i>";
    sendTelegramDirect(resp, chatId);
  }

  // --- คำสั่ง /node1 หรือ /1 ---
  else if (cmd == "/node1" || cmd == "/1" || cmd == "/node_1") {
    NodeInfo n;
    bool found = false;
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
      n = nodes[0];
      found = true;
      xSemaphoreGive(dataMutex);
    }
    if (found) {
      String resp = "📍 <b>[ข้อมูล Node #1: " + String(n.name) + "]</b>\n";
      resp += "• <b>สถานะเชื่อมต่อ:</b> " + String(n.online ? "✅ ออนไลน์ (Online)" : "❌ ออฟไลน์ (Offline)") + "\n";
      if (n.online) {
        String floodTxt = (n.floodStatus == FLOOD_CRITICAL) ? "🚨 วิกฤต (CRITICAL)" : (n.floodStatus == FLOOD_WARNING) ? "⚠️ เฝ้าระวัง (WARNING)" : "🟢 ปลอดภัย (SAFE)";
        resp += "• <b>ระยะห่างผิวน้ำ:</b> <b>" + String(n.waterLevelCm) + " cm</b> (" + String(n.waterPercent) + "%)\n";
        resp += "• <b>การเตือนภัย:</b> " + floodTxt + "\n";
        resp += "• <b>เกณฑ์ตั้งค่า:</b> ปลอดภัย > " + String(n.warnThresholdCm) + " cm | เฝ้าระวัง ≤ " + String(n.warnThresholdCm) + " cm | วิกฤต ≤ " + String(n.critThresholdCm) + " cm\n";
        resp += "• <b>สัญญาณ LoRa:</b> RSSI " + String(n.rssi) + " dBm | SNR " + String(n.snr, 1) + " dB\n";
        resp += "• <b>แพ็กเกจที่รับได้:</b> " + String(n.packetsReceived) + " ครั้ง\n";
        unsigned long secAgo = (n.lastSeenMillis > 0) ? (millis() - n.lastSeenMillis) / 1000 : 0;
        resp += "• <b>อัปเดตล่าสุด:</b> " + String(secAgo) + " วินาทีที่แล้ว";
      } else {
        resp += "• <b>สาเหตุ:</b> ตรวจไม่พบสัญญาณตอบกลับจากโหนด";
      }
      sendTelegramDirect(resp, chatId);
    }
  }

  // --- คำสั่ง /node2 หรือ /2 ---
  else if (cmd == "/node2" || cmd == "/2" || cmd == "/node_2") {
    NodeInfo n;
    bool found = false;
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
      n = nodes[1];
      found = true;
      xSemaphoreGive(dataMutex);
    }
    if (found) {
      String resp = "📍 <b>[ข้อมูล Node #2: " + String(n.name) + "]</b>\n";
      resp += "• <b>สถานะเชื่อมต่อ:</b> " + String(n.online ? "✅ ออนไลน์ (Online)" : "❌ ออฟไลน์ (Offline)") + "\n";
      if (n.online) {
        String floodTxt = (n.floodStatus == FLOOD_CRITICAL) ? "🚨 วิกฤต (CRITICAL)" : (n.floodStatus == FLOOD_WARNING) ? "⚠️ เฝ้าระวัง (WARNING)" : "🟢 ปลอดภัย (SAFE)";
        resp += "• <b>ระยะห่างผิวน้ำ:</b> <b>" + String(n.waterLevelCm) + " cm</b> (" + String(n.waterPercent) + "%)\n";
        resp += "• <b>การเตือนภัย:</b> " + floodTxt + "\n";
        resp += "• <b>เกณฑ์ตั้งค่า:</b> ปลอดภัย > " + String(n.warnThresholdCm) + " cm | เฝ้าระวัง ≤ " + String(n.warnThresholdCm) + " cm | วิกฤต ≤ " + String(n.critThresholdCm) + " cm\n";
        resp += "• <b>สัญญาณ LoRa:</b> RSSI " + String(n.rssi) + " dBm | SNR " + String(n.snr, 1) + " dB\n";
        resp += "• <b>แพ็กเกจที่รับได้:</b> " + String(n.packetsReceived) + " ครั้ง\n";
        unsigned long secAgo = (n.lastSeenMillis > 0) ? (millis() - n.lastSeenMillis) / 1000 : 0;
        resp += "• <b>อัปเดตล่าสุด:</b> " + String(secAgo) + " วินาทีที่แล้ว";
      } else {
        resp += "• <b>สาเหตุ:</b> ตรวจไม่พบสัญญาณตอบกลับจากโหนด";
      }
      sendTelegramDirect(resp, chatId);
    }
  }

  // --- คำสั่ง /poll หรือ /read ---
  else if (cmd == "/poll" || cmd == "/read" || cmd == "/refresh") {
    triggerManualPoll = true;
    String resp = "🔄 <b>[สั่งการอ่านค่า LoRa Polling ทันที]</b>\n";
    resp += "Master กำลังส่งสัญญาณเรียกอ่านค่าจาก Node ทุกตัว...\n";
    resp += "<i>พิมพ์ /status อีกครั้งเพื่อดูค่าที่อัปเดต</i>";
    sendTelegramDirect(resp, chatId);
  }

  // --- คำสั่ง /thresholds หรือ /limit ---
  else if (cmd == "/thresholds" || cmd == "/threshold" || cmd == "/limit" || cmd == "/set") {
    String resp = "⚙️ <b>[เกณฑ์ระดับน้ำเตือนภัย (เซนเซอร์วัดจากบนลงผิวน้ำ)]</b>\n";
    resp += "━━━━━━━━━━━━━━━━━━━\n";
    for (int i = 0; i < PROTOCOL_MAX_NODES; i++) {
      resp += "📍 <b>Node #" + String(nodes[i].id) + " (" + String(nodes[i].name) + "):</b>\n";
      resp += "  • 🟢 <b>ปลอดภัย (Safe):</b> ระยะห่าง > " + String(nodes[i].warnThresholdCm) + " cm\n";
      resp += "  • ⚠️ <b>เฝ้าระวัง (Warn):</b> ระยะห่าง ≤ " + String(nodes[i].warnThresholdCm) + " cm\n";
      resp += "  • 🚨 <b>วิกฤต (Critical):</b> ระยะห่าง ≤ " + String(nodes[i].critThresholdCm) + " cm\n";
    }
    resp += "━━━━━━━━━━━━━━━━━━━\n";
    resp += "💡 <i>สามารถปรับเปลี่ยนค่าเกณฑ์ได้ผ่านทางหน้า Web Dashboard</i>";
    sendTelegramDirect(resp, chatId);
  }

  // --- คำสั่ง /ip หรือ /dashboard หรือ /web ---
  else if (cmd == "/ip" || cmd == "/dashboard" || cmd == "/web" || cmd == "/url") {
    String resp = "🌐 <b>[ลิงก์เข้าดู Mobile Web Dashboard]</b>\n";
    resp += "• <b>IP Address:</b> http://" + WiFi.localIP().toString() + "\n";
    resp += "• <b>mDNS Hostname:</b> http://floodmonitor.local\n";
    resp += "• <b>WiFi Network:</b> " + String(WiFi.SSID()) + " (" + String(WiFi.RSSI()) + " dBm)\n";
    resp += "<i>(กรุณาเชื่อมต่อ WiFi วงเดียวกันก่อนเปิดลิงก์)</i>";
    sendTelegramDirect(resp, chatId);
  }

  // --- คำสั่ง /help หรือ /start หรือ /menu ---
  else if (cmd == "/help" || cmd == "/start" || cmd == "/menu") {
    String resp = "🤖 <b>[ระบบบอทตรวจเช็คน้ำท่วม ESP32-S3 Master]</b>\n";
    resp += "ยินดีต้อนรับคุณ " + fromUser + "! ท่านสามารถพิมพ์คำสั่งต่อไปนี้:\n\n";
    resp += "📊 <b>/status</b> - ดูสถานะภาพรวมและระดับน้ำทุกโหนด\n";
    resp += "📍 <b>/node1</b> - ดูข้อมูลละเอียด Node #1\n";
    resp += "📍 <b>/node2</b> - ดูข้อมูลละเอียด Node #2\n";
    resp += "🔄 <b>/poll</b> - สั่งยิง LoRa Polling ไปยังทุกโหนดทันที\n";
    resp += "⚙️ <b>/thresholds</b> - ดูเกณฑ์ระดับน้ำเตือนภัยที่ตั้งไว้\n";
    resp += "🌐 <b>/ip</b> - ดู IP Address และลิงก์หน้า Dashboard\n";
    resp += "❓ <b>/help</b> - แสดงคู่มือการใช้งานคำสั่งทั้งหมด";
    sendTelegramDirect(resp, chatId);
  }

  // --- กรณีพิมพ์คำสั่งอื่นที่ไม่รู้จัก ---
  else if (cmd.startsWith("/")) {
    String resp = "❓ <b>ไม่พบคำสั่ง '" + cmd + "'</b>\n";
    resp += "กรุณาพิมพ์ <b>/help</b> เพื่อดูรายการคำสั่งทั้งหมดที่รองรับ";
    sendTelegramDirect(resp, chatId);
  }
}

// =========================================================================
// ตรวจเช็คข้อความ/คำสั่งใหม่จาก Telegram Bot API (getUpdates)
// =========================================================================
void handleTelegramUpdates() {
  if (WiFi.status() != WL_CONNECTED || strlen(TELEGRAM_BOT_TOKEN) == 0) return;
  if (ESP.getFreeHeap() < 25000) {
    Serial.printf("[Telegram Bot] Low heap memory (%d bytes). Skipping getUpdates.\n", ESP.getFreeHeap());
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(5000);
  client.setHandshakeTimeout(5);

  HTTPClient https;
  https.setTimeout(5000);
  https.setReuse(false);

  String url = "";
  if (isFirstUpdateCheck) {
    // ครั้งแรกหลังบูต: ดึงเฉพาะ update ล่าสุดตัวเดียวเพื่อกำหนด offset ไม่ค้างกับข้อความเก่า
    url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + "/getUpdates?offset=-1&limit=1&timeout=0";
  } else {
    url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + "/getUpdates?offset=" + String((long long)(lastTelegramUpdateId + 1)) + "&limit=5&timeout=0";
  }

  if (https.begin(client, url)) {
    int httpCode = https.GET();
    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
      String payload = https.getString();
      https.end();
      client.stop(); // คืน Socket ทันที

      if (payload.length() > 0) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);

        if (!err && doc["ok"] == true) {
          JsonArray results = doc["result"].as<JsonArray>();

          if (isFirstUpdateCheck) {
            for (JsonObject item : results) {
              int64_t uid = item["update_id"];
              if (uid >= lastTelegramUpdateId) {
                lastTelegramUpdateId = uid;
              }
            }
            isFirstUpdateCheck = false;
            Serial.printf("[Telegram Bot] Offset initialized to %lld (Skipped historical backlog)\n", (long long)lastTelegramUpdateId);
            return;
          }

          // ประมวลผลคำสั่งใหม่ที่เข้ามา
          for (JsonObject item : results) {
            int64_t uid = item["update_id"];
            if (uid >= lastTelegramUpdateId) {
              lastTelegramUpdateId = uid;
            }

            if (item["message"].is<JsonObject>()) {
              JsonObject msg = item["message"];
              if (msg["text"].is<const char*>()) {
                const char* text = msg["text"];
                String chatId = msg["chat"]["id"].as<String>();
                const char* fromName = msg["from"]["first_name"];

                if (text != NULL && text[0] == '/') {
                  processTelegramCommand(String(text), chatId, fromName ? String(fromName) : "User");
                }
              }
            }
          }
        }
      }
    } else {
      https.end();
      client.stop();
    }
  } else {
    https.end();
    client.stop();
  }
}

void TaskTelegram(void *pvParameters) {
  Serial.println(F("[FreeRTOS] Telegram Notifier Task started on Core 0"));

  initTelegramNotifier();

  TelegramMessage msg;
  unsigned long lastUpdateCheck = 0;

  for (;;) {
    // 1. ตรวจเช็คคิวส่งข้อความแจ้งเตือนอัตโนมัติ (Timeout 250ms)
    if (xQueueReceive(telegramQueue, &msg, pdMS_TO_TICKS(250)) == pdTRUE) {
      int retryWait = 0;
      while (WiFi.status() != WL_CONNECTED && retryWait < 10) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        retryWait++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        sendTelegramDirect(String(msg.text), String(msg.chatId));
      } else {
        Serial.println(F("[Telegram] Dropped message: WiFi not available."));
      }

      vTaskDelay(pdMS_TO_TICKS(500));
    }

    // 2. ตรวจเช็คคำสั่งที่ผู้ใช้พิมพ์เข้ามาใน Telegram ทุกๆ 2.5 วินาที
    if (millis() - lastUpdateCheck >= 2500) {
      lastUpdateCheck = millis();
      if (WiFi.status() == WL_CONNECTED) {
        handleTelegramUpdates();
      }
    }
  }
}

void checkNodeTelegramAlert(uint8_t nodeIndex, uint8_t newFloodStatus, bool newOnline) {
  if (!TELEGRAM_ENABLED) return;
  if (nodeIndex >= PROTOCOL_MAX_NODES) return;

  uint8_t nodeId = 0;
  String nodeName = "";
  uint16_t levelCm = 0;
  uint8_t percent = 0;
  int rssi = 0;
  float snr = 0.0;
  uint8_t prevStatus = FLOOD_NORMAL;
  bool prevOnline = false;
  unsigned long lastAlert = 0;

  if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    nodeId = nodes[nodeIndex].id;
    nodeName = String(nodes[nodeIndex].name);
    levelCm = nodes[nodeIndex].waterLevelCm;
    percent = nodes[nodeIndex].waterPercent;
    rssi = nodes[nodeIndex].rssi;
    snr = nodes[nodeIndex].snr;
    prevStatus = nodes[nodeIndex].prevFloodStatus;
    prevOnline = nodes[nodeIndex].prevOnline;
    lastAlert = nodes[nodeIndex].lastAlertMillis;
    xSemaphoreGive(dataMutex);
  } else {
    return;
  }

  unsigned long now = millis();
  bool shouldAlert = false;
  uint8_t alertPriority = TELE_PRIORITY_NORMAL;
  String message = "";

  // 1. ตรวจสอบกรณีโหนดขาดการเชื่อมต่อ (Online -> Offline)
  if (prevOnline && !newOnline && TELEGRAM_ALERT_OFFLINE) {
    shouldAlert = true;
    alertPriority = TELE_PRIORITY_URGENT;
    message = "⚠️ <b>[แจ้งเตือนโหนดขาดการเชื่อมต่อ]</b> ⚠️\n";
    message += "📍 <b>จุดตรวจวัด:</b> Node #" + String(nodeId) + " (" + nodeName + ")\n";
    message += "❌ <b>สถานะ:</b> ไม่ตอบสนอง (OFFLINE)\n";
    message += "⏱ <b>รายละเอียด:</b> ตรวจไม่พบสัญญาณตอบกลับหลังส่งซ้ำครบจำนวน";
  }
  // 2. ตรวจสอบกรณีโหนดกลับมาเชื่อมต่อ (Offline -> Online)
  else if (!prevOnline && newOnline && TELEGRAM_ALERT_ONLINE) {
    shouldAlert = true;
    alertPriority = TELE_PRIORITY_NORMAL;
    message = "📡 <b>[โหนดกลับมาเชื่อมต่อสำเร็จ]</b>\n";
    message += "📍 <b>จุดตรวจวัด:</b> Node #" + String(nodeId) + " (" + nodeName + ")\n";
    message += "✅ <b>สถานะ:</b> ออนไลน์ (ONLINE)\n";
    message += "📏 <b>ระดับน้ำ:</b> " + String(levelCm) + " cm (" + String(percent) + "%)\n";
    message += "📶 <b>สัญญาณ LoRa:</b> RSSI " + String(rssi) + " dBm | SNR " + String(snr, 1) + " dB";
  }
  // 3. ตรวจสอบกรณีเกิดภาวะวิกฤตน้ำท่วม (Critical Alert)
  else if (newOnline && newFloodStatus == FLOOD_CRITICAL && TELEGRAM_ALERT_CRITICAL) {
    if (prevStatus != FLOOD_CRITICAL || (now - lastAlert >= TELEGRAM_CRITICAL_REMIND_MS)) {
      shouldAlert = true;
      alertPriority = TELE_PRIORITY_URGENT;
      message = "🚨🚨🚨 <b>[เตือนภัยวิกฤตน้ำท่วม!]</b> 🚨🚨🚨\n";
      message += "📍 <b>จุดตรวจวัด:</b> Node #" + String(nodeId) + " (" + nodeName + ")\n";
      message += "📏 <b>ระดับน้ำปัจจุบัน:</b> <b>" + String(levelCm) + " cm (" + String(percent) + "%)</b>\n";
      message += "📊 <b>สถานะ:</b> 🚨 วิกฤตน้ำท่วม (CRITICAL)\n";
      message += "📶 <b>สัญญาณ LoRa:</b> RSSI " + String(rssi) + " dBm | SNR " + String(snr, 1) + " dB\n";
      message += "⚠️ <i>โปรดเฝ้าระวังและเตรียมพร้อมรับมือน้ำท่วมทันที!</i>";
    }
  }
  // 4. ตรวจสอบกรณีระดับน้ำเข้าสู่สภาวะเฝ้าระวัง (Warning Alert)
  else if (newOnline && newFloodStatus == FLOOD_WARNING && prevStatus != FLOOD_WARNING && TELEGRAM_ALERT_WARNING) {
    shouldAlert = true;
    alertPriority = TELE_PRIORITY_URGENT;
    message = "⚠️ <b>[แจ้งเตือนระดับน้ำ: เฝ้าระวัง]</b>\n";
    message += "📍 <b>จุดตรวจวัด:</b> Node #" + String(nodeId) + " (" + nodeName + ")\n";
    message += "📏 <b>ระดับน้ำปัจจุบัน:</b> " + String(levelCm) + " cm (" + String(percent) + "%)\n";
    message += "📊 <b>สถานะ:</b> ⚠️ เฝ้าระวัง (WARNING)\n";
    message += "📶 <b>สัญญาณ LoRa:</b> RSSI " + String(rssi) + " dBm | SNR " + String(snr, 1) + " dB";
  }
  // 5. ตรวจสอบกรณีระดับน้ำลดลงกลับสู่ระดับปลอดภัย (Recovery Alert)
  else if (newOnline && newFloodStatus == FLOOD_NORMAL && (prevStatus == FLOOD_WARNING || prevStatus == FLOOD_CRITICAL) && TELEGRAM_ALERT_RECOVERY) {
    shouldAlert = true;
    alertPriority = TELE_PRIORITY_NORMAL;
    message = "✅ <b>[ระดับน้ำลดลงสู่ระดับปลอดภัย (SAFE)]</b>\n";
    message += "📍 <b>จุดตรวจวัด:</b> Node #" + String(nodeId) + " (" + nodeName + ")\n";
    message += "📏 <b>ระยะห่างผิวน้ำ:</b> " + String(levelCm) + " cm\n";
    message += "📊 <b>สถานะ:</b> 🟢 ปลอดภัย (SAFE)";
  }

  // อัปเดตสถานะและเวลาส่งเตือน
  if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    nodes[nodeIndex].prevFloodStatus = newFloodStatus;
    nodes[nodeIndex].prevOnline = newOnline;
    if (shouldAlert) {
      nodes[nodeIndex].lastAlertMillis = now;
    }
    xSemaphoreGive(dataMutex);
  }

  if (shouldAlert && message.length() > 0) {
    queueTelegramMessage(message, "", alertPriority);
  }
}
