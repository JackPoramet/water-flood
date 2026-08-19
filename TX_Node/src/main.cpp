#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "Config.h"
#include "Protocol.h"
#include "FloodSensor.h"

uint8_t packetSequence = 0;
unsigned long bootMillis = 0;

void sendDataResponse() {
  digitalWrite(PIN_LED, HIGH);

  SensorPayload payload;
  generateFloodData(MY_NODE_ID, bootMillis, payload);

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
    return;
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
    
    delay(TURNAROUND_DELAY_MS);
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

  LoRa.setTxPower(LORA_TX_POWER, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setSyncWord(LORA_SYNC_WORD);
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