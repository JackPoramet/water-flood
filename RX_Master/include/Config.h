#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =========================================================================
// 1. กำหนด Pin สำหรับบอร์ด ESP32-S3
// =========================================================================

// --- I2C OLED Display (SSD1306 128x64) ---
#define OLED_SDA            5
#define OLED_SCL            4
#define OLED_ADDR           0x3C
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64

// --- LoRa RA-02 (SX1278 433MHz) SPI ---
#define LORA_SCK            12
#define LORA_MISO           13
#define LORA_MOSI           11
#define LORA_CS             10   // NSS
#define LORA_RST            -1   // ขา RST ต่อ 3.3V
#define LORA_DIO0           -1   // ไม่ต้องต่อ (ใช้ Polling)

// =========================================================================
// 2. การตั้งค่าคลื่นวิทยุ LoRa เพื่อระยะทางไกลสูงสุด (Maximum Long-Range Profile: SF12)
// =========================================================================
#define LORA_BAND           433E6 // 433.0 MHz
#define LORA_SF             12    // Spreading Factor 12 (ความไวภาครับสูงสุด -148dBm ส่งได้ไกลที่สุด ทะลุสิ่งกีดขวาง)
#define LORA_BW             125E3 // Bandwidth 125 kHz
#define LORA_CR             8     // Coding Rate 4/8 (แก้ไขข้อผิดพลาดของสัญญาณสูงสุด)
#define LORA_SYNC_WORD      0x12  // Sync Word เครือข่ายเฉพาะกลุ่ม
#define LORA_TX_POWER       20    // กำลังส่งสูงสุด 20 dBm (100mW PA_BOOST)

// =========================================================================
// 3. การตั้งค่า WiFi (Station Mode)
// =========================================================================
extern const char* WIFI_SSID;
extern const char* WIFI_PASS;

// =========================================================================
// 4. พารามิเตอร์ระบบ LoRa Polling สำหรับ SF12
// =========================================================================
#define POLL_TIMEOUT_MS     3500  // เวลารอการตอบกลับ 3.5 วินาที (ครอบคลุม Airtime ของ SF12 อย่างสมบูรณ์)
#define CYCLE_INTERVAL_MS   2000  // เว้นระยะระหว่างรอบ Polling 2.0 วินาที
#define MAX_RETRIES         2     // จำนวนครั้งที่ส่งซ้ำเมื่อ Node ไม่ตอบ

// =========================================================================
// 5. การตั้งค่าระบบแจ้งเตือน Telegram Group (Telegram Bot API)
// =========================================================================
#define TELEGRAM_ENABLED            true   // เปิด/ปิด การส่งแจ้งเตือน Telegram (true = เปิดใช้งาน)
extern const char* TELEGRAM_BOT_TOKEN;     // Telegram Bot Token (ได้จาก @BotFather)
extern const char* TELEGRAM_CHAT_ID;       // Group Chat ID (เช่น -100xxxxxxxxxx หรือ -xxxxxxxx)

#define TELEGRAM_ALERT_BOOT         true   // แจ้งเตือนเมื่อ Master บูตและต่อ WiFi สำเร็จ
#define TELEGRAM_ALERT_CRITICAL     true   // แจ้งเตือนเมื่อระดับน้ำเข้าสู่วิกฤต (สีแดง)
#define TELEGRAM_ALERT_WARNING      true   // แจ้งเตือนเมื่อระดับน้ำเข้าสู่สภาวะเฝ้าระวัง (สีเหลือง)
#define TELEGRAM_ALERT_RECOVERY     true   // แจ้งเตือนเมื่อระดับน้ำลดลงกลับสู่ปกติ (สีเขียว)
#define TELEGRAM_ALERT_OFFLINE      true   // แจ้งเตือนเมื่อโหนดขาดการเชื่อมต่อ (Offline)
#define TELEGRAM_ALERT_ONLINE       true   // แจ้งเตือนเมื่อโหนดกลับมาเชื่อมต่อได้ (Online)
#define TELEGRAM_CRITICAL_REMIND_MS (30UL * 60UL * 1000UL) // เตือนซ้ำกรณีวิกฤตต่อเนื่องทุก 30 นาที

// =========================================================================
// 6. ค่าเริ่มต้นเกณฑ์ระยะห่างผิวน้ำเตือนภัย (Top-Down Sensor Distance in cm)
//    - เซนเซอร์ติดตั้งด้านบนยิงลงผิวน้ำ (ระยะยิ่งน้อย = น้ำยิ่งสูง):
//      1. ปลอดภัย (Safe)   : ระยะห่าง > DEFAULT_WARN_THRESHOLD_CM (> 300 cm)
//      2. เฝ้าระวัง (Warn) : ระยะห่าง <= DEFAULT_WARN_THRESHOLD_CM (<= 300 cm และ > 200 cm)
//      3. วิกฤต (Critical) : ระยะห่าง <= DEFAULT_CRIT_THRESHOLD_CM (<= 200 cm)
// =========================================================================
#define DEFAULT_WARN_THRESHOLD_CM   300   // ระยะห่างผิวน้ำเริ่มเฝ้าระวัง (cm)
#define DEFAULT_CRIT_THRESHOLD_CM   200   // ระยะห่างผิวน้ำเข้าขั้นวิกฤต (cm)

#endif // CONFIG_H
