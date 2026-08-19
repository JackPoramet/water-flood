# 🌊 ระบบตรวจเช็คน้ำท่วมไร้สาย Multi-Node LoRa + FreeRTOS Dual-Core Web Dashboard

ระบบตรวจวัดและแจ้งเตือนภัยระดับน้ำท่วมแบบไร้สายระยะไกล (LoRa 433MHz) โดยใช้ **ESP32-S3 Dual-Core FreeRTOS (Core 0: WiFi WebServer & mDNS, Core 1: LoRa Polling + OLED)** และ **ATmega32U4 (LoRa32u4) 3 ตัวเป็นโหนดตรวจวัดระดับน้ำ (Node 1, 2, 3)** สื่อสารผ่านโปรโตคอล **Master-Initiated Polling** ป้องกันการชนกันของสัญญาณวิทยุ (Collision-Free) พร้อมหน้าเว็บ **Mobile Dashboard** แบบ Real-time

---

## 📝 บันทึกการเปลี่ยนแปลงล่าสุด (Changelog)

| วันที่ | ส่วนที่แก้ไข | รายละเอียดการปรับปรุง |
|---|---|---|
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
    │   ├── Config.h               <-- การตั้งค่า Pin, RF, Node ID, และ Turnaround Delay
    │   ├── Protocol.h             <-- โครงสร้าง Binary Protocol ส่วนกลาง
    │   └── FloodSensor.h          <-- ส่วนคำนวณและจำลองข้อมูลระดับน้ำ
    ├── src/
    │   ├── FloodSensor.cpp        <-- ตรรกะจำลองระดับน้ำและแรงดันแบตเตอรี่
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
