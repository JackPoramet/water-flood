#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "Config.h"
#include "Protocol.h"
#include "FloodSensor.h"

uint8_t packetSequence = 0;
unsigned long bootMillis = 0;

#if defined(USE_MODBUS_SENSOR) || defined(USE_JSN_SR04T) || defined(USE_HC_SR04)
// ตัวแปรสำหรับ Periodic Sensor Reading (Node 1, 2)
unsigned long lastSensorReadMs = 0;
SensorPayload latestSensorData; // เก็บค่าล่าสุดจากเซนเซอร์จริง
bool           sensorDataReady = false;
#endif

// ฟังก์ชันช่วยควบคุม LED
inline void setStatusLed(uint8_t state) {
  digitalWrite(PIN_LED, state);
}

void sendDataResponse() {
  setStatusLed(HIGH);

  SensorPayload payload;

#if defined(USE_MODBUS_SENSOR) || defined(USE_JSN_SR04T) || defined(USE_HC_SR04)
  // ใช้ค่าจริงจากเซนเซอร์ที่อ่านไว้ล่าสุด (หากมี)
  if (sensorDataReady) {
    memcpy(&payload, &latestSensorData, sizeof(SensorPayload));
  } else {
    readFloodSensor(MY_NODE_ID, bootMillis, payload);
  }
#else
  readFloodSensor(MY_NODE_ID, bootMillis, payload);
#endif

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

  setStatusLed(LOW);

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

#if defined(USE_MODBUS_SENSOR)
  initFloodSensor();
  Serial.println(F("[+] Sensor Mode: RS485 Modbus RTU (DJLK-003AB)"));
#elif defined(USE_JSN_SR04T)
  initFloodSensor();
  Serial.println(F("[+] Sensor Mode: Ultrasonic (JSN-SR04T)"));
#elif defined(USE_HC_SR04)
  initFloodSensor();
  Serial.println(F("[+] Sensor Mode: Ultrasonic (HC-SR04)"));
#else
  Serial.println(F("[+] Sensor Mode: Simulated Data"));
#endif

  LoRa.setPins(PIN_LORA_SS, PIN_LORA_RST, PIN_LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println(F("[-] LoRa begin failed! Check Pins & Wiring"));
    while (1) {
      setStatusLed(HIGH);
      delay(150);
      setStatusLed(LOW);
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
    setStatusLed(HIGH);
    delay(100);
    setStatusLed(LOW);
    delay(100);
  }
}

void loop() {
#if defined(USE_MODBUS_SENSOR) || defined(USE_JSN_SR04T) || defined(USE_HC_SR04)
  // อ่านค่าจากเซนเซอร์เป็นระยะ (ตาม SENSOR_READ_INTERVAL_MS)
  unsigned long now = millis();
  if (now - lastSensorReadMs >= SENSOR_READ_INTERVAL_MS) {
    lastSensorReadMs = now;
    sensorDataReady = readFloodSensor(MY_NODE_ID, bootMillis, latestSensorData);
  }
#endif

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    handleIncomingPacket(packetSize);
  }
}