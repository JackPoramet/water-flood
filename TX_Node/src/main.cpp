#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "Protocol.h"

// =========================================================================
// กำหนด ID ประจำตัว Node (หากไม่ระบุใน build_flags จะใช้ Node ID = 1 เป็นค่าเริ่มต้น)
// =========================================================================
#ifndef NODE_ID
  #define NODE_ID NODE_ID_1
#endif

const uint8_t MY_NODE_ID = NODE_ID;

// Pin Definitions สำหรับบอร์ด LoRa32u4 (ATmega32U4)
#define PIN_LORA_SS    8
#define PIN_LORA_RST   4
#define PIN_LORA_DIO0  7
#define PIN_LED        13

#define LORA_BAND      433E6  // 433 MHz

uint8_t packetSequence = 0;
unsigned long bootMillis = 0;

// ฟังก์ชันจำลองค่าระดับน้ำตาม Node ID
void generateMockData(SensorPayload &payload) {
  unsigned long now = millis();
  float t = (now - bootMillis) / 1000.0;

  if (MY_NODE_ID == NODE_ID_1) {
    // Node 1: ระดับน้ำปกติ (35 - 75 cm)
    payload.waterLevelCm = (uint16_t)(55.0 + 20.0 * sin(t / 25.0));
  } else if (MY_NODE_ID == NODE_ID_2) {
    // Node 2: เฝ้าระวัง (120 - 170 cm)
    payload.waterLevelCm = (uint16_t)(145.0 + 25.0 * sin(t / 18.0));
  } else {
    // Node 3: วิกฤตน้ำท่วม (215 - 280 cm)
    payload.waterLevelCm = (uint16_t)(245.0 + 35.0 * sin(t / 12.0));
  }

  payload.waterPercent = constrain(map(payload.waterLevelCm, 0, 300, 0, 100), 0, 100);

  if (payload.waterLevelCm >= 200) {
    payload.floodStatus = FLOOD_CRITICAL;
  } else if (payload.waterLevelCm >= 100) {
    payload.floodStatus = FLOOD_WARNING;
  } else {
    payload.floodStatus = FLOOD_NORMAL;
  }

  payload.batteryMilliVolt = (uint16_t)(4050 - (t / 120.0));
  if (payload.batteryMilliVolt < 3500) payload.batteryMilliVolt = 3500;

  payload.uptimeSec = (uint16_t)((now - bootMillis) / 1000);
}

// ฟังก์ชันส่งข้อมูลเซนเซอร์ตอบกลับ Master
void sendDataResponse() {
  digitalWrite(PIN_LED, HIGH);

  SensorPayload payload;
  generateMockData(payload);

  PacketHeader header;
  header.magic      = PROTOCOL_MAGIC_BYTE;
  header.targetId   = NODE_ID_MASTER;
  header.senderId   = MY_NODE_ID;
  header.msgType    = MSG_DATA_RESP;
  header.seqNum     = packetSequence++;
  header.payloadLen = sizeof(SensorPayload);

  size_t totalLen = sizeof(PacketHeader) + sizeof(SensorPayload);
  uint8_t buffer[sizeof(PacketHeader) + sizeof(SensorPayload) + 2];

  memcpy(buffer, &header, sizeof(PacketHeader));
  memcpy(buffer + sizeof(PacketHeader), &payload, sizeof(SensorPayload));

  uint16_t crc = calculateCRC16(buffer, totalLen);
  buffer[totalLen]     = (uint8_t)(crc & 0xFF);
  buffer[totalLen + 1] = (uint8_t)((crc >> 8) & 0xFF);

  LoRa.beginPacket();
  LoRa.write(buffer, totalLen + 2);
  LoRa.endPacket();

  digitalWrite(PIN_LED, LOW);

  Serial.print(F("[TX->Master] Pkt #"));
  Serial.print(header.seqNum);
  Serial.print(F(" Sent: Water="));
  Serial.print(payload.waterLevelCm);
  Serial.print(F("cm ("));
  Serial.print(payload.waterPercent);
  Serial.print(F("%), Status="));
  Serial.print(payload.floodStatus);
  Serial.print(F(", Bat="));
  Serial.print(payload.batteryMilliVolt);
  Serial.print(F("mV, Uptime="));
  Serial.print(payload.uptimeSec);
  Serial.println(F("s"));
}

// ฟังก์ชันประมวลผลแพ็กเกจที่ได้รับจาก Master
void handleIncomingPacket(int packetSize) {
  if (packetSize < (int)(sizeof(PacketHeader) + 2)) {
    return;
  }

  uint8_t rxBuffer[48];
  int bytesRead = 0;
  while (LoRa.available() && bytesRead < (int)sizeof(rxBuffer)) {
    rxBuffer[bytesRead++] = LoRa.read();
  }

  PacketHeader* header = (PacketHeader*)rxBuffer;
  if (header->magic != PROTOCOL_MAGIC_BYTE) {
    return;
  }

  if (header->targetId != MY_NODE_ID && header->targetId != NODE_ID_BROADCAST) {
    return; // ไม่ใช่คำสั่งสำหรับ Node นี้
  }

  size_t expectedLen = sizeof(PacketHeader) + header->payloadLen;
  if (bytesRead < (int)(expectedLen + 2)) {
    return;
  }

  uint16_t receivedCrc = rxBuffer[expectedLen] | (rxBuffer[expectedLen + 1] << 8);
  uint16_t computedCrc = calculateCRC16(rxBuffer, expectedLen);

  if (receivedCrc != computedCrc) {
    Serial.println(F("[-] CRC Mismatch on received packet!"));
    return;
  }

  if (header->msgType == MSG_POLL_REQ) {
    Serial.print(F("\n[RX<-Master] Poll Request Received for Node #"));
    Serial.print(MY_NODE_ID);
    Serial.print(F("! (RSSI: "));
    Serial.print(LoRa.packetRssi());
    Serial.println(F(" dBm)"));
    
    // หน่วงเวลา 100ms เพื่อให้ Master เคลียร์และเข้าโหมด RX สมบูรณ์
    delay(100);
    sendDataResponse();
  }
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  Serial.begin(115200);
  bootMillis = millis();

  unsigned long startWait = millis();
  while (!Serial && (millis() - startWait < 2500)) {
    delay(10);
  }

  Serial.println(F("\n=================================================="));
  Serial.print  (F("   LoRa32u4 Flood Sensor Node #"));
  Serial.print  (MY_NODE_ID);
  Serial.println(F(" Initializing... "));
  Serial.println(F("=================================================="));

  LoRa.setPins(PIN_LORA_SS, PIN_LORA_RST, PIN_LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println(F("[-] LoRa begin failed! Check Pins & Wiring"));
    while (1) {
      digitalWrite(PIN_LED, HIGH);
      delay(150);
      digitalWrite(PIN_LED, LOW);
      delay(150);
    }
  }

  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(8);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();

  Serial.println(F("[+] LoRa Initialized successfully!"));
  Serial.print  (F("[+] Node #"));
  Serial.print  (MY_NODE_ID);
  Serial.println(F(" in Standby Listening Mode (Waiting for Master Poll)...\n"));

  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(100);
    digitalWrite(PIN_LED, LOW);
    delay(100);
  }
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    handleIncomingPacket(packetSize);
  }
}