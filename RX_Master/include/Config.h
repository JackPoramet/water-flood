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
// 2. การตั้งค่าคลื่นวิทยุ LoRa เพื่อระยะทางไกลสูงสุด (Maximum Long-Range Profile)
// =========================================================================
#define LORA_BAND           433E6 // 433.0 MHz
#define LORA_SF             12    // Spreading Factor 12 (ความไวภาครับสูงสุด -148dBm ส่งได้ไกลที่สุด)
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
// 4. พารามิเตอร์ระบบ LoRa Polling สำหรับโหมดระยะไกล (Long-Range Timing)
// =========================================================================
#define POLL_TIMEOUT_MS     3500  // เวลารอการตอบกลับ 3.5 วินาที (ครอบคลุม Airtime ของ SF12 อย่างสมบูรณ์)
#define CYCLE_INTERVAL_MS   2000  // เว้นระยะระหว่างรอบ Polling 2.0 วินาที
#define MAX_RETRIES         2     // จำนวนครั้งที่ส่งซ้ำเมื่อ Node ไม่ตอบ

#endif // CONFIG_H
