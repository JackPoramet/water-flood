#ifndef TELEGRAM_NOTIFIER_H
#define TELEGRAM_NOTIFIER_H

#include <Arduino.h>

// ความยาวสูงสุดของข้อความในคิว
#define TELEGRAM_MAX_MSG_LEN 768
#define TELEGRAM_MAX_CHAT_LEN 32

// ระดับความสำคัญของข้อความ Telegram
#define TELE_PRIORITY_NORMAL  0  // ข้อความทั่วไป (Boot, Online, Recovery)
#define TELE_PRIORITY_URGENT  1  // ข้อความเร่งด่วน (Warning, Critical, Offline)

struct TelegramMessage {
  char text[TELEGRAM_MAX_MSG_LEN];
  char chatId[TELEGRAM_MAX_CHAT_LEN];
  uint8_t priority;  // 0 = Normal, 1 = Urgent (แทรกหัวคิว)
};

// ฟังก์ชันเริ่มต้นโมดูลและคิว Telegram
void initTelegramNotifier();

// ฟังก์ชันส่งข้อความเข้าคิว (Non-blocking ปลอดภัยกับ LoRa Task)
bool queueTelegramMessage(const String& text, const String& targetChatId = "", uint8_t priority = TELE_PRIORITY_NORMAL);

// ฟังก์ชันตรวจสอบการเปลี่ยนสถานะของโหนดและส่งเตือนเข้า Telegram อัตโนมัติ
void checkNodeTelegramAlert(uint8_t nodeIndex, uint8_t newFloodStatus, bool newOnline);

// FreeRTOS Task สำหรับประมวลผลคิว รับคำสั่ง และยิง HTTPS POST ไปยัง Telegram Bot API
void TaskTelegram(void *pvParameters);

// ฟังก์ชันยิงข้อความตรงแบบ Synchronous
bool sendTelegramDirect(const String& text, const String& targetChatId = "");

// ฟังก์ชันตรวจเช็คคำสั่งที่ผู้ใช้พิมพ์เข้ามาใน Telegram (getUpdates)
void handleTelegramUpdates();

// ฟังก์ชันประมวลผลคำสั่งที่ได้รับจาก Telegram
void processTelegramCommand(String cmd, const String& chatId, const String& fromUser);

#endif // TELEGRAM_NOTIFIER_H
