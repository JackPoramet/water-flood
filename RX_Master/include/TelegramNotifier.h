#ifndef TELEGRAM_NOTIFIER_H
#define TELEGRAM_NOTIFIER_H

#include <Arduino.h>

// ความยาวสูงสุดของข้อความในคิว
#define TELEGRAM_MAX_MSG_LEN 512

struct TelegramMessage {
  char text[TELEGRAM_MAX_MSG_LEN];
};

// ฟังก์ชันเริ่มต้นโมดูลและคิว Telegram
void initTelegramNotifier();

// ฟังก์ชันส่งข้อความเข้าคิว (Non-blocking ปลอดภัยกับ LoRa Task)
bool queueTelegramMessage(const String& text);

// ฟังก์ชันตรวจสอบการเปลี่ยนสถานะของโหนดและส่งเตือนเข้า Telegram อัตโนมัติ
void checkNodeTelegramAlert(uint8_t nodeIndex, uint8_t newFloodStatus, bool newOnline);

// FreeRTOS Task สำหรับประมวลผลคิวและยิง HTTPS POST ไปยัง Telegram Bot API
void TaskTelegram(void *pvParameters);

// ฟังก์ชันยิงข้อความตรงแบบ Synchronous (ใช้ภายใน TaskTelegram)
bool sendTelegramDirect(const String& text);

#endif // TELEGRAM_NOTIFIER_H
