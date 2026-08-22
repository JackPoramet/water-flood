#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "Protocol.h"

// =========================================================================
// 1. กำหนด ID ประจำตัว Node
// =========================================================================
#ifndef NODE_ID
  #define NODE_ID NODE_ID_1
#endif

const uint8_t MY_NODE_ID = NODE_ID;

// =========================================================================
// 2. Pin Definitions สำหรับบอร์ด LoRa32u4 (ATmega32U4)
// =========================================================================
#define PIN_LORA_SS         8
#define PIN_LORA_RST        4
#define PIN_LORA_DIO0       7
#define PIN_LED             13

// =========================================================================
// 3. การตั้งค่าคลื่นวิทยุ LoRa เพื่อระยะทางไกลสูงสุด (Maximum Long-Range Profile: SF12)
// =========================================================================
#define LORA_BAND           433E6   // 433.0 MHz
#define LORA_SF             12      // Spreading Factor 12 (ความไวภาครับสูงสุด ส่งได้ไกลที่สุด ทะลุสิ่งกีดขวาง)
#define LORA_BW             125E3   // Bandwidth 125 kHz
#define LORA_CR             8       // Coding Rate 4/8 (แก้ไขข้อผิดพลาดของสัญญาณสูงสุด)
#define LORA_SYNC_WORD      0x12    // Sync Word เครือข่ายเฉพาะกลุ่ม
#define LORA_TX_POWER       20      // กำลังส่งสูงสุด 20 dBm (100mW PA_BOOST)

// =========================================================================
// 4. พารามิเตอร์ Timing สำหรับการตอบกลับโหมดระยะไกล SF12
// =========================================================================
#define TURNAROUND_DELAY_MS 100     // หน่วงเวลา 100ms เพื่อให้ Master เข้าสู่โหมด RX สมบูรณ์ก่อนส่งคลื่น SF12

// =========================================================================
// 5. Pin Definitions สำหรับ MAX485 RS485 Transceiver (เซนเซอร์ DJLK-003AB)
//    ใช้ Serial1 (Hardware UART) ของ ATmega32U4 : RX=D0, TX=D1
// =========================================================================
#ifdef USE_MODBUS_SENSOR
  #define PIN_RS485_RE          6       // Receiver Enable (Active LOW)
  #define PIN_RS485_DE          9       // Driver Enable (Active HIGH)
  #define MODBUS_SLAVE_ID       1       // Slave Address ของ DJLK-003AB (ค่าเริ่มต้น)
  #define MODBUS_BAUD_RATE      9600    // Baud Rate: 9600, 8N1
  #define MODBUS_REG_ADDR       0x0100  // Register Address สำหรับค่าระยะทาง (mm)
  #define SENSOR_READ_INTERVAL_MS 1000  // อ่านค่าจากเซนเซอร์ทุก 1 วินาที
#endif

// =========================================================================
// 6. Pin Definitions สำหรับ Ultrasonic Sensor (Node 2)
//    - TRIG = A3, ECHO = A4
// =========================================================================
#ifdef USE_JSN_SR04T
  #define PIN_JSN_TRIG          A3      // Trigger Pin (A3)
  #define PIN_JSN_ECHO          A4      // Echo Pin (A4)
  #define SENSOR_READ_INTERVAL_MS 1000  // อ่านค่าจากเซนเซอร์ทุก 1 วินาที
#endif

#ifdef USE_HC_SR04
  #define PIN_HC_TRIG           A3      // Trigger Pin (A3)
  #define PIN_HC_ECHO           A4      // Echo Pin (A4)
  #define SENSOR_READ_INTERVAL_MS 1000  // อ่านค่าจากเซนเซอร์ทุก 1 วินาที
#endif

#endif // CONFIG_H
