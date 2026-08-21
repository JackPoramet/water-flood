# 🌊 ระบบตรวจเช็คน้ำท่วมไร้สาย Multi-Node LoRa + FreeRTOS Dual-Core Web Dashboard

ระบบตรวจวัดและแจ้งเตือนภัยระดับน้ำท่วมแบบไร้สายระยะไกล (LoRa 433MHz) โดยใช้ **ESP32-S3 Dual-Core FreeRTOS (Core 0: WiFi WebServer & mDNS, Core 1: LoRa Polling + OLED)** และ **ATmega32U4 (LoRa32u4) 3 ตัวเป็นโหนดตรวจวัดระดับน้ำ (Node 1, 2, 3)** สื่อสารผ่านโปรโตคอล **Master-Initiated Polling** ป้องกันการชนกันของสัญญาณวิทยุ (Collision-Free) พร้อมหน้าเว็บ **Mobile Dashboard** แบบ Real-time

---

## 📝 บันทึกการเปลี่ยนแปลงล่าสุด (Changelog)

| วันที่ | ส่วนที่แก้ไข | รายละเอียดการปรับปรุง |
|---|---|---|
| 2026-08-21 | `RX_Master` & `TX_Node` | **ปรับแต่ง Spreading Factor กลับมาเป็น SF12 (Maximum Long-Range Profile):**<br>1. ปรับ `LORA_SF = 12` ทั้งฝั่ง Master และ Node (ความไวภาครับสูงสุด -148dBm ส่งได้ระยะทางไกลสูงสุด ทะลุสิ่งกีดขวาง)<br>2. ปรับ `POLL_TIMEOUT_MS = 3500` และ `CYCLE_INTERVAL_MS = 2000` เพื่อรองรับ Airtime ของ SF12 อย่างสมบูรณ์<br>3. ปรับ `TURNAROUND_DELAY_MS = 100` บน Node |
| 2026-08-21 | `TX_Node` (ทุก Node) | **ปรับความถี่การอ่านค่าเซนเซอร์ทุกตัวเป็น 1 วินาที (`SENSOR_READ_INTERVAL_MS = 1000`):**<br>1. Node 1 (DJLK-003AB Modbus RTU): อ่านทุก 1 วินาที<br>2. Node 2 (JSN-SR04T Waterproof Ultrasonic): อ่านทุก 1 วินาที<br>3. Node 3 (HC-SR04 Ultrasonic): อ่านทุก 1 วินาที |
| 2026-08-21 | `RX_Master` & `TX_Node` | **ปรับแต่ง Spreading Factor เป็น SF8 (Fast & Balanced Profile):**<br>1. ปรับ `LORA_SF = 8` ทั้งฝั่ง Master และ Node (ลดเวลา Airtime รับส่งข้อมูลเหลือเพียง ~50-80ms ต่อแพ็กเกจ)<br>2. ปรับลด `POLL_TIMEOUT_MS` เหลือ 1.2 วินาที และ `CYCLE_INTERVAL_MS` เหลือ 1.0 วินาที เพิ่มความเร็วการ Polling ตอบสนองแบบ Real-time<br>3. ปรับลด `TURNAROUND_DELAY_MS` บน Node เหลือ 50ms |
| 2026-08-21 | `RX_Master` & `TX_Node` | **ตัดส่วนตรวจเช็คแรงดันแบตเตอรี่ (Battery Voltage) ออก:**<br>1. ตัดการอ่านค่า ADC A9 ใน `FloodSensor.cpp` และตัดการแสดงผลแรงดันแบตเตอรี่ในหน้าเว็บ Dashboard (`WebDashboard.h`)<br>2. ปรับการแสดงผลหน้าเว็บให้เป็นการ์ดระดับความสูงน้ำแบบเต็มความกว้าง (Clean Full-Width Metric)<br>3. ปรับ Serial log ของโหนดไม่ให้แสดงค่าแรงดันแบตเตอรี่ |
| 2026-08-20 | `TX_Node` (`env:node1`) | **เปลี่ยนเป็น ModbusMaster Library + Hardware UART Serial1:**<br>1. ย้ายสาย RO→Pin 0 (RX1), DI→Pin 1 (TX1) เพื่อใช้ Hardware UART แทน Bit-Bang<br>2. ใช้ `ModbusMaster` library (4-20ma v2.0.1) + Callback DE/RE สำหรับ Half-Duplex<br>3. อ่านค่า Register 0x0101 (Real-time 100ms) และ 0x0100 (Processed 500ms)<br>4. RE→Pin 6, DE→Pin 9 ยังคงเดิม | `platformio.ini`, `Config.h`, `HcSr04Manager.h/cpp`, `FloodSensor.h/cpp`, `main.cpp` |
| 2026-08-21 | `TX_Node` (env:node3) | **เพิ่มเซนเซอร์อัลตร้าโซนิค HC-SR04:**<br>1. พัฒนาโมดูล `HcSr04Manager` (Trig=D10, Echo=D11) คำนวณระยะทาง PulseIn พร้อมระบบ Auto-Retry<br>2. รองรับสลับโหมดผ่าน Conditional Flag (`USE_HC_SR04`)<br>3. **ไฟล์ที่เกี่ยวข้อง:** `platformio.ini`, `Config.h`, `HcSr04Manager.h/cpp`, `FloodSensor.h/cpp`, `main.cpp` |
| 2026-08-21 | `TX_Node` (env:node2) | **เพิ่มเซนเซอร์อัลตร้าโซนิคกันน้ำ JSN-SR04T:**<br>1. พัฒนาโมดูล `JsnSr04tManager` (Trig=D10, Echo=D11) คำนวณระยะทางจากสัญญาณสะท้อน PulseIn พร้อมระบบ Auto-Retry<br>2. รองรับสลับโหมดผ่าน Conditional Flag (`USE_JSN_SR04T`)<br>3. **ไฟล์ที่เกี่ยวข้อง:** `platformio.ini`, `Config.h`, `JsnSr04tManager.h/cpp`, `FloodSensor.h/cpp`, `main.cpp` |
| 2026-08-21 | `TX_Node` (env:node1) | **เพิ่มเซนเซอร์ DJLK-003AB (RS485 Modbus RTU):**<br>1. เปลี่ยน Node 1 จากข้อมูลจำลองเป็นอ่านค่าจริงจากเซนเซอร์ Ultrasonic DJLK-003AB ผ่าน MAX485<br>2. ใช้ `ModbusMaster` library + `Serial1` (Hardware UART) อ่าน Register `0x0100` ได้ค่าระยะทาง (mm)<br>3. ใช้ Conditional Compile (`USE_MODBUS_SENSOR`) เฉพาะ env:node1, Node 2/3 ยังใช้ข้อมูลจำลอง<br>4. **ไฟล์ที่แก้ไข:** `platformio.ini`, `Config.h`, `FloodSensor.h`, `FloodSensor.cpp`, `main.cpp` |
| 2026-08-19 | `RX_Master/include/WebDashboard.h` | **ปรับปรุงหน้าเว็บ Dashboard เป็น Minimalist White Theme:**<br>1. ตัดอิโมจิ (Emoji) ทั้งหมดออกจากหน้าเว็บ เพื่อความเรียบง่ายและเป็นทางการ<br>2. เปลี่ยนพื้นหลังเป็นโทนสีขาวสะอาดตา (Clean Light Mode Theme)<br>3. ตัดแอนิเมชันและเอฟเฟกต์ที่ Overengineered ออก คงเหลือเฉพาะข้อมูลสำคัญที่จำเป็นต่อการตรวจเช็ค |
| 2026-08-19 | `RX_Master` & `TX_Node` | **ปรับแต่งโปรไฟล์สำหรับระยะทางไกลสูงสุด (Maximum Long-Range Profile):** SF12, CR 4/8, กำลังส่ง 20dBm, Timeout 3500ms, Turnaround 100ms |
| 2026-08-19 | ทั้งโปรเจกต์ | **จัดโครงสร้างไฟล์โปรเจกต์เป็น Hierarchy / Modular Architecture:** แยกโมดูล `Config.h`, `SystemState.h`, `Protocol.h`, `LoRaEngine`, `WebPortal`, `WebDashboard.h`, และ `DisplayManager` |

---

## 📱 วิธีการเข้าดูหน้าเว็บ Mobile Dashboard

1. ตรวจสอบว่าสมาร์ทโฟนหรือคอมพิวเตอร์เชื่อมต่อ **WiFi วงเดียวกันกับที่ ESP32-S3 เชื่อมต่อ (เช่น CoE#01)**
2. ดู IP Address ที่ปรากฏบน **หน้าจอ OLED** บรรทัดที่ 2 ของ ESP32-S3 (เช่น `IP: 192.168.1.150`)
3. เปิดเว็บเบราว์เซอร์แล้วพิมพ์:
   $$\text{\bf http://<IP_ที่แสดงบนหน้าจอOLED>} \quad \text{(เช่น http://192.168.1.150)}$$
4. หน้าจอจะแสดง Dashboard คลีนโทนสีขาว แสดงระดับน้ำ แบตเตอรี่ และสถานะเตือนภัยของทุก Node แบบ Real-time

---

## 📁 โครงสร้างโปรเจกต์แบบโมดูลาร์ (Project File Hierarchy)

```
water_flood/
├── README.md                      <-- เอกสารคู่มือระบบและสรุปการทำงาน
├── AGENTS.md                      <-- กฎข้อบังคับการพัฒนาและบันทึกเอกสาร
├── water_flood.code-workspace     <-- ไฟล์ Workspace รวมโปรเจกต์ของ VS Code
│
├── RX_Master/                     <-- [ESP32-S3 Master + WebServer]
│   ├── include/
│   │   ├── Config.h               <-- การตั้งค่า Pin, WiFi, RF และ Timing กลาง
│   │   ├── Protocol.h             <-- โครงสร้าง Binary Frame, Header, CRC16
│   │   ├── SystemState.h          <-- ตัวแปรสถานะส่วนกลาง, Mutex, โครงสร้าง Node
│   │   ├── WebDashboard.h         <-- หน้าเว็บ Clean White Theme (No Emojis)
│   │   ├── WebPortal.h            <-- โมดูลจัดการ WiFi และ HTTP Server
│   │   ├── LoRaEngine.h           <-- โมดูล LoRa Polling State Machine
│   │   └── DisplayManager.h       <-- โมดูลจัดการหน้าจอ OLED SSD1306
│   ├── src/
│   │   ├── WebPortal.cpp          <-- ซอร์สโค้ด Web Server Task (Core 0)
│   │   ├── LoRaEngine.cpp         <-- ซอร์สโค้ด LoRa Polling Task (Core 1)
│   │   ├── DisplayManager.cpp     <-- ซอร์สโค้ด OLED Display Task (Core 1)
│   │   └── main.cpp               <-- จุดเริ่มต้นโปรแกรม (สร้าง Mutex & RTOS Tasks สั้นกระชับ)
│   └── platformio.ini
│
└── TX_Node/                       <-- [LoRa32u4 Sensor Node (Node 1, 2, 3)]
    ├── include/
    │   ├── Config.h               <-- การตั้งค่า Pin, RF, Node ID, Turnaround Delay + Sensor Pins
    │   ├── Protocol.h             <-- โครงสร้าง Binary Protocol ส่วนกลาง
    │   ├── FloodSensor.h          <-- อินเทอร์เฟซเซนเซอร์ (Unified API: Modbus / JSN / HC / จำลอง)
    │   ├── ModbusManager.h        <-- โมดูล RS485 Modbus RTU (Node 1: MAX485 + DJLK-003AB)
    │   ├── JsnSr04tManager.h      <-- โมดูล Ultrasonic (Node 2: JSN-SR04T Pulse Mode)
    │   └── HcSr04Manager.h        <-- โมดูล Ultrasonic (Node 3: HC-SR04 Pulse Mode)
    ├── src/
    │   ├── FloodSensor.cpp        <-- ตรรกะแปลงค่าและจัดระดับน้ำสำหรับทุก Node
    │   ├── ModbusManager.cpp      <-- จัดการ Serial1, MAX485 DE/RE, ModbusMaster Library
    │   ├── JsnSr04tManager.cpp    <-- จัดการ Trigger/Echo, PulseIn, Auto-Retry (Node 2)
    │   ├── HcSr04Manager.cpp      <-- จัดการ Trigger/Echo, PulseIn, Auto-Retry (Node 3)
    │   └── main.cpp               <-- ลูปสแตนด์บายฟังคำสั่ง Polling และตอบกลับ
    └── platformio.ini
```

---

## ⚡ สถาปัตยกรรม Dual-Core FreeRTOS

```
+---------------------------------------------------------------------------------------+
|                                ESP32-S3 Dual-Core SoC                                 |
|                                                                                       |
|   [ Core 0 ] - Networking & Web Engine           [ Core 1 ] - Radio & UI Engine       |
|   +------------------------------------+         +--------------------------------+   |
|   |  - WiFi Station Handler (WIFI_STA) |         |  - LoRa SX1278 Polling Engine  |   |
|   |  - HTTP WebServer (Port 80)        |         |    (Node 1 -> Node 2 -> Node 3)|   |
|   |  - REST API (/api/data, /api/poll) |         |  - CRC16 & Timeout Handler     |   |
|   |                                    |         |  - I2C OLED (SSD1306 128x64)   |   |
|   +-----------------+------------------+         +----------------+---------------+   |
|                     |                                             |                   |
|                     +-----------------> [ Mutex ] <---------------+                   |
|                                    (Node Telemetry)                                   |
+---------------------------------------------------------------------------------------+
```

---

## 🔌 ตารางการต่อสายฮาร์ดแวร์ (Hardware Pinout)

### 1. ฝั่ง Master: บอร์ด ESP32-S3 DevKitC-1
| อุปกรณ์ | ขาโมดูล | ขา ESP32-S3 | หมายเหตุ |
|---|---|---|---|
| **OLED (SSD1306)** | SDA | **GPIO 5** | I2C Data |
| | SCL | **GPIO 4** | I2C Clock |
| | VCC / GND | 3.3V / GND | ไฟเลี้ยงจอ |
| **LoRa RA-02** | SCK | **GPIO 12** | SPI Clock |
| | MISO | **GPIO 13** | SPI Master In |
| | MOSI | **GPIO 11** | SPI Master Out |
| | NSS / CS | **GPIO 10** | Chip Select |
| | RST | **ต่อ 3.3V** | จั๊มไฟ 3.3V ตลอดเวลา |
| | DIO0 | *ไม่ต้องต่อ* | ใช้ระบบ Polling |
| | VCC / GND | 3.3V / GND | ไฟเลี้ยงโมดูล (3.3V เท่านั้น) |

---

### 2. ฝั่ง Node: บอร์ด LoRa32u4 (ATmega32U4 + SX1278)
| ขาโมดูล LoRa | ขาไมโครคอนโทรลเลอร์ (On-board) |
|---|---|
| NSS / CS | **D8** |
| RESET | **D4** |
| DIO0 | **D7** |
| LED สถานะ | **D13** (กะพริบเมื่อส่ง/รับข้อมูล) |

---

### 3. โมดูล MAX485 ↔ LoRa32u4 (Node 1 — เซนเซอร์ DJLK-003AB)
| ขา MAX485 | ขา LoRa32u4 | หมายเหตุ |
|---|---|---|
| **RO** (Receiver Out) | **D0 / RX** (Serial1 RX) | ข้อมูลจากเซนเซอร์เข้า |
| **DI** (Driver Input) | **D1 / TX** (Serial1 TX) | ข้อมูลส่งไปเซนเซอร์ |
| **RE** (Receiver Enable) | **D6** | Active LOW — ต่ำ = เปิดรับ |
| **DE** (Driver Enable) | **D9** | Active HIGH — สูง = เปิดส่ง |
| **VCC** | **5V** | ไฟเลี้ยง MAX485 |
| **GND** | **GND** | กราวด์ร่วม |
| **A, B** | สาย RS485 → DJLK-003AB | A↔A, B↔B |

> **หมายเหตุ:** เซนเซอร์ DJLK-003AB ใช้ Modbus RTU (9600 8N1), Slave ID=1, Register `0x0100` คืนค่าระยะทาง (mm)

---

### 4. เซนเซอร์ JSN-SR04T ↔ LoRa32u4 (Node 2 — เซนเซอร์ Ultrasonic กันน้ำ)
| ขา JSN-SR04T | ขา LoRa32u4 | หมายเหตุ |
|---|---|---|
| **TRIG** (Trigger) | **D12** | สัญญาณ Pulse เริ่มยิงคลื่น |
| **ECHO** (Echo) | **D5** | สัญญาณ Pulse สะท้อนกลับ (pulseIn) |
| **VCC** | **5V / USB** | ⚠️ ต้องใช้ไฟ 5V เท่านั้น |
| **GND** | **GND** | กราวด์ร่วม |

> **หมายเหตุ:** JSN-SR04T มีพิสัยวัด 20 cm – 600 cm (Dead zone < 20 cm)

---

### 5. เซนเซอร์ HC-SR04 ↔ LoRa32u4 (Node 3 — เซนเซอร์ Ultrasonic ทั่วไป)
| ขา HC-SR04 | ขา LoRa32u4 | หมายเหตุ |
|---|---|---|
| **TRIG** (Trigger) | **D10** | สัญญาณ Pulse เริ่มยิงคลื่น |
| **ECHO** (Echo) | **D11** | สัญญาณ Pulse สะท้อนกลับ |
| **VCC** | **5V** | ไฟเลี้ยงโมดูล (5V) |
| **GND** | **GND** | กราวด์ร่วม |

> **หมายเหตุ:** HC-SR04 มีพิสัยวัด 2 cm – 400 cm (ความแม่นยำสูงในระยะใกล้)

---

## 🚀 วิธีการอัปโหลดโค้ด (Upload Guide)

### 1. อัปโหลดบอร์ด Master (ESP32-S3)
```bash
cd RX_Master
pio run --target upload --upload-port COM12
```

### 2. อัปโหลดบอร์ด Node (LoRa32u4)
- **สำหรับ Node 1 (คลองระบายน้ำ):**
  ```bash
  cd TX_Node
  pio run -e node1 --target upload --upload-port COM22
  ```
- **สำหรับ Node 2 (ริมแม่น้ำเฝ้าระวัง):**
  ```bash
  cd TX_Node
  pio run -e node2 --target upload --upload-port COM22
  ```
- **สำหรับ Node 3 (จุดเสี่ยงน้ำท่วมชุมชน):**
  ```bash
  cd TX_Node
  pio run -e node3 --target upload --upload-port <COM_PORT>
  ```
