#include "LoRaEngine.h"
#include <SPI.h>
#include <LoRa.h>
#include "Config.h"
#include "Protocol.h"
#include "SystemState.h"
#include "TelegramNotifier.h"

static int retryCount = 0;
static unsigned long pollStartMillis = 0;
static unsigned long cycleStartMillis = 0;
static uint8_t masterSeqNum = 0;

static void sendPollPacket(uint8_t targetNodeId) {
  PacketHeader header;
  header.magic      = PROTOCOL_MAGIC_BYTE;
  header.targetId   = targetNodeId;
  header.senderId   = NODE_ID_MASTER;
  header.msgType    = MSG_POLL_REQ;
  header.seqNum     = masterSeqNum++;
  header.payloadLen = 0;

  uint8_t buffer[sizeof(PacketHeader) + 2];
  memcpy(buffer, &header, sizeof(PacketHeader));

  uint16_t crc = calculateCRC16(buffer, sizeof(PacketHeader));
  buffer[sizeof(PacketHeader)]     = (uint8_t)(crc & 0xFF);
  buffer[sizeof(PacketHeader) + 1] = (uint8_t)((crc >> 8) & 0xFF);

  LoRa.beginPacket();
  LoRa.write(buffer, sizeof(PacketHeader) + 2);
  LoRa.endPacket();

  Serial.printf("\n[Master->Node#%d] POLL_REQ Sent (Seq: %d, Retry: %d)...\n", 
                targetNodeId, header.seqNum, retryCount);
}

static void parseLoRaResponse(int packetSize) {
  Serial.printf("[DEBUG RX] Raw LoRa packet received! Size = %d bytes, RSSI = %d dBm\n", 
                packetSize, LoRa.packetRssi());

  if (packetSize < (int)(sizeof(PacketHeader) + 2)) {
    Serial.println(F("[-] Packet too short."));
    return;
  }

  uint8_t rxBuffer[64];
  int bytesRead = 0;
  while (LoRa.available() && bytesRead < (int)sizeof(rxBuffer)) {
    rxBuffer[bytesRead++] = LoRa.read();
  }

  PacketHeader* header = (PacketHeader*)rxBuffer;
  if (header->magic != PROTOCOL_MAGIC_BYTE) {
    Serial.printf("[-] Magic byte mismatch: 0x%02X\n", header->magic);
    return;
  }

  if (header->targetId != NODE_ID_MASTER) {
    Serial.printf("[-] Target ID mismatch: %d\n", header->targetId);
    return;
  }

  size_t expectedLen = sizeof(PacketHeader) + header->payloadLen;
  if (bytesRead < (int)(expectedLen + 2)) {
    Serial.printf("[-] Payload length mismatch: bytesRead=%d < expected=%d\n", bytesRead, expectedLen + 2);
    return;
  }

  uint16_t receivedCrc = rxBuffer[expectedLen] | (rxBuffer[expectedLen + 1] << 8);
  uint16_t computedCrc = calculateCRC16(rxBuffer, expectedLen);
  if (receivedCrc != computedCrc) {
    Serial.printf("[-] CRC Mismatch: Recv 0x%04X != Calc 0x%04X\n", receivedCrc, computedCrc);
    return;
  }

  if (header->msgType == MSG_DATA_RESP) {
    uint8_t sender = header->senderId;
    if (sender >= 1 && sender <= PROTOCOL_MAX_NODES) {
      uint8_t idx = sender - 1;
      SensorPayload* payload = (SensorPayload*)(rxBuffer + sizeof(PacketHeader));

      if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        nodes[idx].online           = true;
        nodes[idx].waterLevelCm     = payload->waterLevelCm;
        
        // คำนวณ floodStatus เมื่อเซนเซอร์วัดระยะจากบนลงผิวน้ำ (ระยะยิ่งน้อย = ผิวน้ำยิ่งสูง)
        uint8_t calculatedStatus = FLOOD_NORMAL;
        if (nodes[idx].critThresholdCm > 0 && payload->waterLevelCm <= nodes[idx].critThresholdCm) {
          calculatedStatus = FLOOD_CRITICAL;
        } else if (nodes[idx].warnThresholdCm > 0 && payload->waterLevelCm <= nodes[idx].warnThresholdCm) {
          calculatedStatus = FLOOD_WARNING;
        } else {
          calculatedStatus = FLOOD_NORMAL; // ปลอดภัย (Safe / Normal)
        }
        nodes[idx].floodStatus = calculatedStatus;

        // คำนวณเปอร์เซ็นต์ระดับน้ำ (ระยะ 0 cm = 100% เต็มตลิ่ง, ระยะ >= maxClearance = 0% ปลอดภัย)
        uint16_t maxClearance = (nodes[idx].warnThresholdCm > 0) ? (uint16_t)(nodes[idx].warnThresholdCm * 1.35) : 400;
        if (payload->waterLevelCm >= maxClearance) {
          nodes[idx].waterPercent = 0;
        } else {
          nodes[idx].waterPercent = constrain(map(payload->waterLevelCm, maxClearance, 0, 0, 100), 0, 100);
        }
        nodes[idx].batteryMilliVolt = payload->batteryMilliVolt;
        nodes[idx].uptimeSec        = payload->uptimeSec;
        nodes[idx].rssi             = LoRa.packetRssi();
        nodes[idx].snr              = LoRa.packetSnr();
        nodes[idx].lastSeenMillis   = millis();
        nodes[idx].packetsReceived++;
        xSemaphoreGive(dataMutex);
      }

      Serial.println(F("=================================================="));
      Serial.printf("[Master<-Node#%d] DATA_RESP SUCCESS!\n", sender);
      Serial.printf("    - Water Distance: %d cm (Risk/Level: %d%%)\n", nodes[idx].waterLevelCm, nodes[idx].waterPercent);
      Serial.printf("    - Flood Status  : %s (Safe > %dcm | Warn <= %dcm | Crit <= %dcm)\n", 
                    (nodes[idx].floodStatus == 2 ? "CRITICAL" : (nodes[idx].floodStatus == 1 ? "WARNING" : "SAFE")),
                    nodes[idx].warnThresholdCm, nodes[idx].warnThresholdCm, nodes[idx].critThresholdCm);
      Serial.printf("    - Battery       : %d mV\n", nodes[idx].batteryMilliVolt);
      Serial.printf("    - Signal        : RSSI %d dBm | SNR %.1f dB\n", nodes[idx].rssi, nodes[idx].snr);
      Serial.println(F("=================================================="));

      // ตรวจสอบและส่งแจ้งเตือน Telegram อัตโนมัติ (Async Queue)
      checkNodeTelegramAlert(idx, nodes[idx].floodStatus, true);

      currentPollState = STATE_ADVANCE_NODE;
    }
  }
}

void TaskLoRaPolling(void *pvParameters) {
  Serial.println(F("[FreeRTOS] LoRa Polling Task started on Core 1"));

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setSPI(SPI);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  LoRa.setSPIFrequency(1000000);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println(F("[-] LoRa begin failed! Retrying in background..."));
    while (!LoRa.begin(LORA_BAND)) {
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }

  LoRa.setTxPower(LORA_TX_POWER, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setSyncWord(LORA_SYNC_WORD);
  LoRa.enableCrc();

  Serial.println(F("[+] LoRa Polling Engine Ready on Core 1 (SF7 Fast Mode)!"));

  for (;;) {
    unsigned long now = millis();

    if (triggerManualPoll) {
      triggerManualPoll = false;
      currentNodeIndex = 0;
      retryCount = 0;
      currentPollState = STATE_SEND_POLL;
    }

    switch (currentPollState) {
      case STATE_SEND_POLL: {
        uint8_t targetId = nodes[currentNodeIndex].id;
        sendPollPacket(targetId);
        pollStartMillis = now;
        currentPollState = STATE_WAIT_RESPONSE;
        break;
      }

      case STATE_WAIT_RESPONSE: {
        int packetSize = LoRa.parsePacket();
        if (packetSize) {
          parseLoRaResponse(packetSize);
        }

        if (currentPollState == STATE_WAIT_RESPONSE && (now - pollStartMillis >= POLL_TIMEOUT_MS)) {
          if (retryCount < MAX_RETRIES) {
            retryCount++;
            Serial.printf("[Master] Timeout Node #%d. Retry (%d/%d)...\n", 
                          nodes[currentNodeIndex].id, retryCount, MAX_RETRIES);
            currentPollState = STATE_SEND_POLL;
          } else {
            Serial.printf("[-] Node #%d OFFLINE\n", nodes[currentNodeIndex].id);
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
              nodes[currentNodeIndex].online = false;
              nodes[currentNodeIndex].timeoutCount++;
              xSemaphoreGive(dataMutex);
            }
            // แจ้งเตือนโหนดขาดการเชื่อมต่อเข้า Telegram
            checkNodeTelegramAlert(currentNodeIndex, nodes[currentNodeIndex].floodStatus, false);

            currentPollState = STATE_ADVANCE_NODE;
          }
        }
        break;
      }

      case STATE_ADVANCE_NODE: {
        retryCount = 0;
        currentNodeIndex++;
        if (currentNodeIndex >= PROTOCOL_MAX_NODES) {
          currentNodeIndex = 0;
          pollCycleCount++;
          cycleStartMillis = now;
          currentPollState = STATE_CYCLE_WAIT;
          Serial.printf("\n=== [Poll Cycle #%d Completed] ===\n\n", pollCycleCount);
        } else {
          currentPollState = STATE_SEND_POLL;
        }
        break;
      }

      case STATE_CYCLE_WAIT: {
        if (now - cycleStartMillis >= CYCLE_INTERVAL_MS) {
          currentPollState = STATE_SEND_POLL;
        }
        break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
