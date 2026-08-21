#ifndef LORA_PROTOCOL_H
#define LORA_PROTOCOL_H

#include <Arduino.h>

// =========================================================================
// ข้อกำหนดโปรโตคอลมาตรฐาน LoRa Multi-Node Flood Monitoring Protocol
// =========================================================================

#define PROTOCOL_MAGIC_BYTE   0xAA
#define PROTOCOL_MAX_NODES    2

// Node IDs
#define NODE_ID_MASTER        0x00
#define NODE_ID_1             0x01
#define NODE_ID_2             0x02
#define NODE_ID_BROADCAST     0xFF

// Message Types / Commands
enum MessageType : uint8_t {
  MSG_POLL_REQ  = 0x01,  // Master -> Node : ขอข้อมูลสถานะ
  MSG_DATA_RESP = 0x02,  // Node -> Master : ตอบกลับข้อมูลเซนเซอร์
  MSG_ACK       = 0x03   // การตอบรับแพ็กเกจ
};

// สถานะระดับน้ำเตือนภัย
enum FloodStatus : uint8_t {
  FLOOD_NORMAL   = 0x00, // ปกติ (น้ำ < 100cm)
  FLOOD_WARNING  = 0x01, // เฝ้าระวัง (น้ำ 100 - 200cm)
  FLOOD_CRITICAL = 0x02  // วิกฤติน้ำท่วม (น้ำ > 200cm)
};

#pragma pack(push, 1)

// โครงสร้างข้อมูลเซนเซอร์ (Payload) 8 bytes
struct SensorPayload {
  uint16_t waterLevelCm;     // ระดับน้ำ (เซนติเมตร) เช่น 45, 150, 240
  uint8_t  waterPercent;     // ระดับน้ำคิดเป็นเปอร์เซ็นต์ (0 - 100%)
  uint8_t  floodStatus;      // สถานะเตือนภัย (FloodStatus)
  uint16_t batteryMilliVolt; // แรงดันแบตเตอรี่ (mV) เช่น 3700, 4150
  uint16_t uptimeSec;        // ระยะเวลาที่ทำงาน (วินาที)
};

// โครงสร้างส่วนหัวของแพ็กเกจ (Packet Header) 6 bytes
struct PacketHeader {
  uint8_t magic;      // 0xAA (Protocol Magic Byte)
  uint8_t targetId;   // ID ผู้รับ (0x00=Master, 0x01..0x03=Node, 0xFF=Broadcast)
  uint8_t senderId;   // ID ผู้ส่ง
  uint8_t msgType;    // MessageType (POLL_REQ, DATA_RESP, ACK)
  uint8_t seqNum;     // Sequence Number (0 - 255)
  uint8_t payloadLen; // ความยาว Payload
};

#pragma pack(pop)

// ฟังก์ชันคำนวณ CRC16-CCITT (Polynomial 0x1021) เพื่อตรวจสอบความถูกต้องของข้อมูล
inline uint16_t calculateCRC16(const uint8_t* data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= ((uint16_t)data[i] << 8);
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

#endif // LORA_PROTOCOL_H
