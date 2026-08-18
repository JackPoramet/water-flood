#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Protocol.h"

// =========================================================================
// 1. กำหนด Pin และการตั้งค่าฮาร์ดแวร์สำหรับ ESP32-S3
// =========================================================================
#define OLED_SDA        5
#define OLED_SCL        4
#define OLED_ADDR       0x3C
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64

#define LORA_SCK        12
#define LORA_MISO       13
#define LORA_MOSI       11
#define LORA_CS         10
#define LORA_RST        -1   // ขา RST ต่อไฟ 3.3V
#define LORA_DIO0       -1

#define LORA_BAND       433E6 // 433 MHz

// =========================================================================
// 2. การตั้งค่า WiFi (Station Mode)
// =========================================================================
const char* WIFI_SSID = "CoE#01";     // ชื่อ WiFi ของท่าน
const char* WIFI_PASS = "xxxxxxxx"; // รหัสผ่าน WiFi

// พารามิเตอร์ระบบ Polling
const unsigned long POLL_TIMEOUT_MS = 3500;    // เวลารอการตอบกลับจาก Node (3.5 วินาที สำหรับ SF12)
const unsigned long CYCLE_INTERVAL_MS = 5000;  // เว้นระยะระหว่างรอบ Polling (5 วินาที)
const int MAX_RETRIES = 2;                      // จำนวนครั้งที่ส่งซ้ำเมื่อ Node ไม่ตอบ

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);

// =========================================================================
// 3. โครงสร้างข้อมูลสถานะของแต่ละ Node
// =========================================================================
struct NodeInfo {
  uint8_t id;
  const char* name;
  bool online;
  uint16_t waterLevelCm;
  uint8_t waterPercent;
  uint8_t floodStatus;
  uint16_t batteryMilliVolt;
  uint16_t uptimeSec;
  int rssi;
  float snr;
  unsigned long lastSeenMillis;
  uint32_t packetsReceived;
  uint32_t timeoutCount;
};

NodeInfo nodes[PROTOCOL_MAX_NODES] = {
  {NODE_ID_1, "จุดที่ 1 (คลองระบายน้ำ)", false, 0, 0, FLOOD_NORMAL, 0, 0, 0, 0.0, 0, 0, 0},
  {NODE_ID_2, "จุดที่ 2 (ริมแม่น้ำเฝ้าระวัง)", false, 0, 0, FLOOD_NORMAL, 0, 0, 0, 0.0, 0, 0, 0},
  {NODE_ID_3, "จุดที่ 3 (จุดเสี่ยงน้ำท่วมชุมชน)", false, 0, 0, FLOOD_NORMAL, 0, 0, 0, 0.0, 0, 0, 0}
};

// สถานะการทำงานของ Polling State Machine
enum PollState {
  STATE_SEND_POLL,
  STATE_WAIT_RESPONSE,
  STATE_ADVANCE_NODE,
  STATE_CYCLE_WAIT
};

PollState currentPollState = STATE_SEND_POLL;
uint8_t currentNodeIndex = 0;
int retryCount = 0;
unsigned long pollStartMillis = 0;
unsigned long cycleStartMillis = 0;
uint32_t pollCycleCount = 0;
uint8_t masterSeqNum = 0;
uint8_t animStep = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastWifiCheck = 0;

// =========================================================================
// 4. ฟังก์ชันระบบ LoRa Polling Protocol
// =========================================================================

void sendPollRequest(uint8_t targetNodeId) {
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

  Serial.printf("\n[Master->Node#%d] POLL_REQ Sent (Seq: %d, Retry: %d). Listening...\n", 
                targetNodeId, header.seqNum, retryCount);
}

void processIncomingPacket(int packetSize) {
  Serial.printf("[DEBUG RX] Raw LoRa packet detected, size = %d bytes, RSSI = %d dBm\n", 
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
    Serial.printf("[-] Payload length error: bytesRead=%d < expected=%d\n", bytesRead, expectedLen + 2);
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

      nodes[idx].online           = true;
      nodes[idx].waterLevelCm     = payload->waterLevelCm;
      nodes[idx].waterPercent     = payload->waterPercent;
      nodes[idx].floodStatus      = payload->floodStatus;
      nodes[idx].batteryMilliVolt = payload->batteryMilliVolt;
      nodes[idx].uptimeSec        = payload->uptimeSec;
      nodes[idx].rssi             = LoRa.packetRssi();
      nodes[idx].snr              = LoRa.packetSnr();
      nodes[idx].lastSeenMillis   = millis();
      nodes[idx].packetsReceived++;

      Serial.println(F("=================================================="));
      Serial.printf("[Master<-Node#%d] DATA_RESP SUCCESS!\n", sender);
      Serial.printf("    - Water Level : %d cm (%d%%)\n", nodes[idx].waterLevelCm, nodes[idx].waterPercent);
      Serial.printf("    - Flood Status: %s\n", (nodes[idx].floodStatus == 2 ? "CRITICAL" : (nodes[idx].floodStatus == 1 ? "WARNING" : "NORMAL")));
      Serial.printf("    - Battery     : %d mV\n", nodes[idx].batteryMilliVolt);
      Serial.printf("    - Signal      : RSSI %d dBm | SNR %.1f dB\n", nodes[idx].rssi, nodes[idx].snr);
      Serial.println(F("=================================================="));

      currentPollState = STATE_ADVANCE_NODE;
    }
  }
}

void updatePollingStateMachine() {
  unsigned long now = millis();

  switch (currentPollState) {
    case STATE_SEND_POLL: {
      uint8_t targetId = nodes[currentNodeIndex].id;
      sendPollRequest(targetId);
      pollStartMillis = now;
      currentPollState = STATE_WAIT_RESPONSE;
      break;
    }

    case STATE_WAIT_RESPONSE: {
      int packetSize = LoRa.parsePacket();
      if (packetSize) {
        processIncomingPacket(packetSize);
      }

      if (currentPollState == STATE_WAIT_RESPONSE && (now - pollStartMillis >= POLL_TIMEOUT_MS)) {
        if (retryCount < MAX_RETRIES) {
          retryCount++;
          Serial.printf("[Master] Timeout waiting for Node #%d. Retrying (%d/%d)...\n", 
                        nodes[currentNodeIndex].id, retryCount, MAX_RETRIES);
          currentPollState = STATE_SEND_POLL;
        } else {
          Serial.printf("[-] Node #%d Failed to Respond -> OFFLINE\n", nodes[currentNodeIndex].id);
          nodes[currentNodeIndex].online = false;
          nodes[currentNodeIndex].timeoutCount++;
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
        Serial.printf("\n=== [Poll Cycle #%d Completed. Next cycle in %lu sec] ===\n\n", 
                      pollCycleCount, CYCLE_INTERVAL_MS / 1000);
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
}

// =========================================================================
// 5. Web Server & Mobile Responsive Dashboard
// =========================================================================

void handleApiData() {
  String json = "{";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"pollCycle\":" + String(pollCycleCount) + ",";
  json += "\"activeNode\":" + String(nodes[currentNodeIndex].id) + ",";
  json += "\"wifiSsid\":\"" + String(WiFi.SSID()) + "\",";
  json += "\"wifiIp\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"wifiRssi\":" + String(WiFi.RSSI()) + ",";
  
  bool anyCritical = false;
  uint16_t maxLevel = 0;
  for (int i = 0; i < PROTOCOL_MAX_NODES; i++) {
    if (nodes[i].floodStatus == FLOOD_CRITICAL) anyCritical = true;
    if (nodes[i].waterLevelCm > maxLevel) maxLevel = nodes[i].waterLevelCm;
  }
  json += "\"floodAlert\":" + String(anyCritical ? "true" : "false") + ",";
  json += "\"maxLevelCm\":" + String(maxLevel) + ",";

  json += "\"nodes\":[";
  for (int i = 0; i < PROTOCOL_MAX_NODES; i++) {
    json += "{";
    json += "\"id\":" + String(nodes[i].id) + ",";
    json += "\"name\":\"" + String(nodes[i].name) + "\",";
    json += "\"online\":" + String(nodes[i].online ? "true" : "false") + ",";
    json += "\"waterLevelCm\":" + String(nodes[i].waterLevelCm) + ",";
    json += "\"waterPercent\":" + String(nodes[i].waterPercent) + ",";
    json += "\"floodStatus\":" + String(nodes[i].floodStatus) + ",";
    json += "\"batteryMv\":" + String(nodes[i].batteryMilliVolt) + ",";
    json += "\"uptimeSec\":" + String(nodes[i].uptimeSec) + ",";
    json += "\"rssi\":" + String(nodes[i].rssi) + ",";
    json += "\"snr\":" + String(nodes[i].snr, 1) + ",";
    unsigned long secAgo = nodes[i].lastSeenMillis > 0 ? (millis() - nodes[i].lastSeenMillis) / 1000 : 9999;
    json += "\"lastSeenSec\":" + String(secAgo) + ",";
    json += "\"packetsReceived\":" + String(nodes[i].packetsReceived);
    json += "}";
    if (i < PROTOCOL_MAX_NODES - 1) json += ",";
  }
  json += "]}";

  server.send(200, "application/json", json);
}

void handleApiPollNow() {
  currentPollState = STATE_SEND_POLL;
  currentNodeIndex = 0;
  retryCount = 0;
  server.send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"Polling triggered\"}");
}

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="th">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ระบบตรวจเช็คน้ำท่วมอัจฉริยะ (Flood Monitor Dashboard)</title>
  <link href="https://fonts.googleapis.com/css2?family=Prompt:wght@300;400;500;600;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg-dark: #0f172a;
      --card-bg: rgba(30, 41, 59, 0.7);
      --card-border: rgba(255, 255, 255, 0.1);
      --primary: #38bdf8;
      --success: #10b981;
      --warning: #f59e0b;
      --danger: #ef4444;
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Prompt', sans-serif; }
    body {
      background: linear-gradient(135deg, #0b1120 0%, #1e1b4b 50%, #0f172a 100%);
      color: var(--text-main);
      min-height: 100vh;
      padding: 16px;
    }
    .container { max-width: 800px; margin: 0 auto; }
    .header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 16px 20px;
      background: var(--card-bg);
      backdrop-filter: blur(12px);
      border: 1px solid var(--card-border);
      border-radius: 16px;
      margin-bottom: 16px;
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
    }
    .header h1 { font-size: 1.25rem; font-weight: 600; color: var(--primary); display: flex; align-items: center; gap: 8px; }
    .header .info { text-align: right; }
    .header .badge {
      font-size: 0.75rem;
      padding: 4px 10px;
      border-radius: 20px;
      background: rgba(56, 189, 248, 0.2);
      color: var(--primary);
      border: 1px solid rgba(56, 189, 248, 0.4);
    }
    .wifi-info { font-size: 0.75rem; color: var(--text-muted); margin-top: 4px; }
    .alert-banner {
      display: none;
      padding: 14px 20px;
      background: linear-gradient(90deg, #dc2626, #991b1b);
      border-radius: 12px;
      margin-bottom: 16px;
      font-weight: 600;
      animation: pulse 1.5s infinite;
      box-shadow: 0 4px 20px rgba(220, 38, 38, 0.5);
    }
    @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.8; } }
    .grid { display: flex; flex-direction: column; gap: 16px; }
    .card {
      background: var(--card-bg);
      backdrop-filter: blur(12px);
      border: 1px solid var(--card-border);
      border-radius: 16px;
      padding: 18px;
      box-shadow: 0 4px 24px rgba(0, 0, 0, 0.25);
      transition: transform 0.2s, border-color 0.2s;
    }
    .card:hover { transform: translateY(-2px); border-color: rgba(56, 189, 248, 0.4); }
    .card-top { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
    .node-title { font-size: 1.05rem; font-weight: 600; }
    .status-tag {
      font-size: 0.8rem;
      font-weight: 500;
      padding: 4px 12px;
      border-radius: 20px;
    }
    .status-normal { background: rgba(16, 185, 129, 0.2); color: #34d399; border: 1px solid rgba(16, 185, 129, 0.4); }
    .status-warning { background: rgba(245, 158, 11, 0.2); color: #fbbf24; border: 1px solid rgba(245, 158, 11, 0.4); }
    .status-critical { background: rgba(239, 68, 68, 0.2); color: #f87171; border: 1px solid rgba(239, 68, 68, 0.4); }
    .status-offline { background: rgba(148, 163, 184, 0.2); color: #94a3b8; border: 1px solid rgba(148, 163, 184, 0.3); }
    
    .metrics { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 14px; }
    .metric-box { background: rgba(15, 23, 42, 0.6); padding: 12px; border-radius: 12px; text-align: center; }
    .metric-val { font-size: 1.6rem; font-weight: 700; color: var(--primary); }
    .metric-lbl { font-size: 0.75rem; color: var(--text-muted); margin-top: 2px; }
    
    .progress-bar-bg {
      background: rgba(15, 23, 42, 0.8);
      height: 10px;
      border-radius: 5px;
      overflow: hidden;
      margin-bottom: 12px;
    }
    .progress-bar-fill {
      height: 100%;
      border-radius: 5px;
      transition: width 0.5s ease-in-out, background-color 0.5s;
    }
    .card-footer {
      display: flex;
      justify-content: space-between;
      font-size: 0.75rem;
      color: var(--text-muted);
      border-top: 1px solid rgba(255, 255, 255, 0.05);
      padding-top: 10px;
    }
    .btn {
      width: 100%;
      padding: 12px;
      background: linear-gradient(135deg, #0284c7, #0369a1);
      color: white;
      border: none;
      border-radius: 12px;
      font-size: 0.95rem;
      font-weight: 600;
      cursor: pointer;
      margin-top: 16px;
      box-shadow: 0 4px 16px rgba(2, 132, 199, 0.4);
      transition: opacity 0.2s;
    }
    .btn:active { opacity: 0.8; }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <div>
        <h1>🌊 ระบบเตือนภัยน้ำท่วม</h1>
        <div class="wifi-info" id="wifi-info">📶 เชื่อมต่อ WiFi...</div>
      </div>
      <div class="info">
        <span class="badge" id="live-indicator">● อัปเดตสด</span>
      </div>
    </div>

    <div class="alert-banner" id="flood-alert">
      ⚠️ แจ้งเตือนด่วน: ตรวจพบระดับน้ำท่วมวิกฤตเกินเกณฑ์ความปลอดภัย!
    </div>

    <div class="grid" id="nodes-container">
      <!-- Node cards will be injected by JavaScript -->
    </div>

    <button class="btn" onclick="triggerPoll()">📡 สะกิดเรียกข้อมูลทุก Node ทันที (Manual Poll)</button>
  </div>

  <script>
    async function fetchData() {
      try {
        const res = await fetch('/api/data');
        const data = await res.json();
        renderDashboard(data);
      } catch (err) {
        console.error("API error:", err);
      }
    }

    function renderDashboard(data) {
      document.getElementById('wifi-info').innerText = `📶 WiFi: ${data.wifiSsid} (${data.wifiIp})`;

      const alertBanner = document.getElementById('flood-alert');
      alertBanner.style.display = data.floodAlert ? 'block' : 'none';

      const container = document.getElementById('nodes-container');
      container.innerHTML = '';

      data.nodes.forEach(node => {
        let statusClass = 'status-offline';
        let statusText = 'ออฟไลน์ (Offline)';
        let barColor = '#64748b';

        if (node.online) {
          if (node.floodStatus === 2) {
            statusClass = 'status-critical';
            statusText = 'วิกฤติน้ำท่วม! (Flood)';
            barColor = 'var(--danger)';
          } else if (node.floodStatus === 1) {
            statusClass = 'status-warning';
            statusText = 'เฝ้าระวัง (Warning)';
            barColor = 'var(--warning)';
          } else {
            statusClass = 'status-normal';
            statusText = 'ปกติ (Normal)';
            barColor = 'var(--success)';
          }
        }

        const card = document.createElement('div');
        card.className = 'card';
        card.innerHTML = `
          <div class="card-top">
            <span class="node-title">${node.name} [Node #${node.id}]</span>
            <span class="status-tag ${statusClass}">${statusText}</span>
          </div>
          <div class="metrics">
            <div class="metric-box">
              <div class="metric-val" style="color: ${barColor}">${node.online ? node.waterLevelCm : '-'} <span style="font-size:0.9rem">cm</span></div>
              <div class="metric-lbl">ระดับความสูงน้ำ</div>
            </div>
            <div class="metric-box">
              <div class="metric-val">${node.online ? (node.batteryMv / 1000).toFixed(2) : '-'} <span style="font-size:0.9rem">V</span></div>
              <div class="metric-lbl">แรงดันแบตเตอรี่</div>
            </div>
          </div>
          <div class="progress-bar-bg">
            <div class="progress-bar-fill" style="width: ${node.online ? node.waterPercent : 0}%; background-color: ${barColor}"></div>
          </div>
          <div class="card-footer">
            <span>📡 RSSI: ${node.online ? node.rssi + ' dBm' : '-'} | SNR: ${node.online ? node.snr + ' dB' : '-'}</span>
            <span>⏱️ ข้อมูลล่าสุด: ${node.online ? node.lastSeenSec + ' วิที่แล้ว' : 'ไม่ตอบสนอง'}</span>
          </div>
        `;
        container.appendChild(card);
      });
    }

    async function triggerPoll() {
      try {
        await fetch('/api/poll', { method: 'POST' });
        fetchData();
      } catch (e) {
        console.error(e);
      }
    }

    setInterval(fetchData, 2500);
    fetchData();
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

// =========================================================================
// 6. การแสดงผลบนหน้าจอ OLED SSD1306 (Multi-Node Dashboard)
// =========================================================================

void updateOLED() {
  display.clearDisplay();

  display.fillRect(0, 0, SCREEN_WIDTH, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print(F("Flood Master"));

  const char animChar[] = {'-', '\\', '|', '/'};
  display.setCursor(76, 2);
  if (WiFi.status() == WL_CONNECTED) {
    display.printf("WF:%c", animChar[animStep % 4]);
  } else {
    display.print(F("WF:NO"));
  }

  display.setCursor(108, 2);
  display.printf("#%d", pollCycleCount % 100);

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 13);
  if (WiFi.status() == WL_CONNECTED) {
    display.print(F("IP:"));
    display.println(WiFi.localIP());
  } else {
    display.println(F("WiFi: Connecting..."));
  }

  const char* statusShort[] = {"OK", "WARN", "ALERT"};
  
  for (int i = 0; i < PROTOCOL_MAX_NODES; i++) {
    int y = 24 + (i * 13);
    display.setCursor(0, y);
    
    if (currentPollState == STATE_WAIT_RESPONSE && currentNodeIndex == i) {
      display.print(F(">N"));
    } else {
      display.print(F(" N"));
    }
    display.print(nodes[i].id);
    display.print(F(":"));

    if (nodes[i].online) {
      display.printf("%3dcm", nodes[i].waterLevelCm);
      display.setCursor(56, y);
      display.printf("[%s]", statusShort[nodes[i].floodStatus % 3]);
      display.setCursor(96, y);
      display.printf("%3ddB", nodes[i].rssi);
    } else {
      display.print(F(" --cm [OFFLINE]"));
    }
  }

  display.display();
}

// =========================================================================
// 7. Setup & Main Loop
// =========================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println(F("\n=================================================="));
  Serial.println(F(" ESP32-S3 Flood Master (WiFi Station + WebServer) "));
  Serial.println(F("=================================================="));

  Wire.begin(OLED_SDA, OLED_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(5, 18);
    display.println(F("ESP32-S3 Master"));
    display.setCursor(5, 32);
    display.println(F("Connecting WiFi..."));
    display.setCursor(5, 46);
    display.println(WIFI_SSID);
    display.display();
  }

  Serial.printf("[+] Connecting to WiFi: %s ...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart < 8000)) {
    delay(300);
    Serial.print(F("."));
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\n[+] WiFi Connected Successfully!"));
    Serial.print(F("[+] IP Address: http://"));
    Serial.println(WiFi.localIP());

    if (MDNS.begin("floodmonitor")) {
      Serial.println(F("[+] mDNS responder started: http://floodmonitor.local"));
    }
  } else {
    Serial.println(F("\n[-] WiFi Connection Timeout! (Will retry in background)"));
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/data", HTTP_GET, handleApiData);
  server.on("/api/poll", HTTP_POST, handleApiPollNow);
  server.begin();
  Serial.println(F("[+] HTTP Web Server Started on Port 80!"));

  Serial.printf("[+] Init SPI: SCK=%d, MISO=%d, MOSI=%d, CS=%d\n", LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setSPI(SPI);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  LoRa.setSPIFrequency(1000000);

  Serial.print(F("[+] Initializing LoRa (433MHz)..."));
  if (!LoRa.begin(LORA_BAND)) {
    Serial.println(F("\n[-] LoRa begin failed! Retrying..."));
    while (!LoRa.begin(LORA_BAND)) {
      Serial.print(F("."));
      delay(500);
    }
  }
  Serial.println(F("\n[+] LoRa SX1278 Initialized Successfully!"));

  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(8);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();

  Serial.println(F("[+] Master Polling Engine Ready!\n"));
  updateOLED();
}

void loop() {
  server.handleClient();
  updatePollingStateMachine();

  if (millis() - lastWifiCheck >= 15000) {
    lastWifiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    }
  }

  if (millis() - lastDisplayUpdate >= 500) {
    lastDisplayUpdate = millis();
    animStep++;
    updateOLED();
  }
}
