#include "TelegramNotifier.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "Config.h"
#include "SystemState.h"

static QueueHandle_t telegramQueue = NULL;

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

bool queueTelegramMessage(const String& text) {
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

  if (xQueueSend(telegramQueue, &msg, 0) == pdPASS) {
    Serial.println(F("[Telegram] Message enqueued successfully."));
    return true;
  } else {
    Serial.println(F("[Telegram] Queue is full! Dropping message."));
    return false;
  }
}

bool sendTelegramDirect(const String& text) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[Telegram] WiFi not connected. Cannot send message."));
    return false;
  }

  if (strlen(TELEGRAM_BOT_TOKEN) == 0 || strlen(TELEGRAM_CHAT_ID) == 0) {
    Serial.println(F("[Telegram] Token or Chat ID empty."));
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10000);

  HTTPClient https;
  https.setTimeout(10000);

  String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage";
  if (https.begin(client, url)) {
    https.addHeader("Content-Type", "application/json; charset=utf-8");

    String payload = "{\"chat_id\":\"" + String(TELEGRAM_CHAT_ID) + "\",\"text\":\"" + jsonEscape(text) + "\",\"parse_mode\":\"HTML\"}";

    int httpCode = https.POST(payload);
    bool success = false;
    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
      Serial.println(F("[Telegram] Notification sent to group successfully!"));
      success = true;
    } else {
      String response = https.getString();
      Serial.printf("[Telegram] HTTP POST failed, code: %d, resp: %s\n", httpCode, response.c_str());
    }
    https.end();
    return success;
  } else {
    Serial.println(F("[Telegram] Failed to connect to api.telegram.org"));
    return false;
  }
}

void TaskTelegram(void *pvParameters) {
  Serial.println(F("[FreeRTOS] Telegram Notifier Task started on Core 0"));

  initTelegramNotifier();

  TelegramMessage msg;
  for (;;) {
    if (xQueueReceive(telegramQueue, &msg, portMAX_DELAY) == pdTRUE) {
      // รอให้ WiFi เชื่อมต่อก่อน
      int retryWait = 0;
      while (WiFi.status() != WL_CONNECTED && retryWait < 10) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        retryWait++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        sendTelegramDirect(String(msg.text));
      }

      // หน่วงเวลาสั้นๆ ป้องกัน Rate Limit จาก Telegram API
      vTaskDelay(pdMS_TO_TICKS(1500));
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
  String message = "";

  // 1. ตรวจสอบกรณีโหนดขาดการเชื่อมต่อ (Online -> Offline)
  if (prevOnline && !newOnline && TELEGRAM_ALERT_OFFLINE) {
    shouldAlert = true;
    message = "⚠️ <b>[แจ้งเตือนโหนดขาดการเชื่อมต่อ]</b> ⚠️\n";
    message += "📍 <b>จุดตรวจวัด:</b> Node #" + String(nodeId) + " (" + nodeName + ")\n";
    message += "❌ <b>สถานะ:</b> ไม่ตอบสนอง (OFFLINE)\n";
    message += "⏱ <b>รายละเอียด:</b> ตรวจไม่พบสัญญาณตอบกลับหลังส่งซ้ำครบจำนวน";
  }
  // 2. ตรวจสอบกรณีโหนดกลับมาเชื่อมต่อ (Offline -> Online)
  else if (!prevOnline && newOnline && TELEGRAM_ALERT_ONLINE) {
    shouldAlert = true;
    message = "📡 <b>[โหนดกลับมาเชื่อมต่อสำเร็จ]</b>\n";
    message += "📍 <b>จุดตรวจวัด:</b> Node #" + String(nodeId) + " (" + nodeName + ")\n";
    message += "✅ <b>สถานะ:</b> ออนไลน์ (ONLINE)\n";
    message += "📏 <b>ระดับน้ำ:</b> " + String(levelCm) + " cm (" + String(percent) + "%)\n";
    message += "📶 <b>สัญญาณ LoRa:</b> RSSI " + String(rssi) + " dBm | SNR " + String(snr, 1) + " dB";
  }
  // 3. ตรวจสอบกรณีเกิดภาวะวิกฤตน้ำท่วม (Critical Alert)
  else if (newOnline && newFloodStatus == FLOOD_CRITICAL && TELEGRAM_ALERT_CRITICAL) {
    // ส่งเมื่อเพิ่งเปลี่ยนเป็น Critical หรือ เตือนซ้ำเมื่อครบระยะเวลา Remind
    if (prevStatus != FLOOD_CRITICAL || (now - lastAlert >= TELEGRAM_CRITICAL_REMIND_MS)) {
      shouldAlert = true;
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
    message = "⚠️ <b>[แจ้งเตือนระดับน้ำ: เฝ้าระวัง]</b>\n";
    message += "📍 <b>จุดตรวจวัด:</b> Node #" + String(nodeId) + " (" + nodeName + ")\n";
    message += "📏 <b>ระดับน้ำปัจจุบัน:</b> " + String(levelCm) + " cm (" + String(percent) + "%)\n";
    message += "📊 <b>สถานะ:</b> ⚠️ เฝ้าระวัง (WARNING)\n";
    message += "📶 <b>สัญญาณ LoRa:</b> RSSI " + String(rssi) + " dBm | SNR " + String(snr, 1) + " dB";
  }
  // 5. ตรวจสอบกรณีระดับน้ำลดลงกลับสู่ระดับปกติ (Recovery Alert)
  else if (newOnline && newFloodStatus == FLOOD_NORMAL && (prevStatus == FLOOD_WARNING || prevStatus == FLOOD_CRITICAL) && TELEGRAM_ALERT_RECOVERY) {
    shouldAlert = true;
    message = "✅ <b>[ระดับน้ำลดลงสู่ระดับปกติ]</b>\n";
    message += "📍 <b>จุดตรวจวัด:</b> Node #" + String(nodeId) + " (" + nodeName + ")\n";
    message += "📏 <b>ระดับน้ำปัจจุบัน:</b> " + String(levelCm) + " cm (" + String(percent) + "%)\n";
    message += "📊 <b>สถานะ:</b> ปกติ (NORMAL)";
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
    queueTelegramMessage(message);
  }
}
