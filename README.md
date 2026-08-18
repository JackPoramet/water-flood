# 🌊 ระบบตรวจเช็คน้ำท่วมไร้สาย Multi-Node LoRa + WiFi Web Dashboard

ระบบตรวจวัดและแจ้งเตือนภัยระดับน้ำท่วมแบบไร้สายระยะไกล (LoRa 433MHz) โดยใช้ **ESP32-S3 เป็น Master Controller & Web Server (เชื่อมต่อ WiFi)** และ **ATmega32U4 (LoRa32u4) 3 ตัวเป็นโหนดตรวจวัดระดับน้ำ (Node 1, 2, 3)** สื่อสารผ่านโปรโตคอล **Master-Initiated Polling** ป้องกันการชนกันของสัญญาณวิทยุ (Collision-Free) พร้อมหน้าเว็บ **Mobile Dashboard** แสดงผลสดแบบ Real-time บนเครือข่าย WiFi เดียวกัน

---

## 📝 บันทึกการเปลี่ยนแปลงล่าสุด (Changelog)

| วันที่ | ส่วนที่แก้ไข | รายละเอียดการปรับปรุง |
|---|---|---|
| 2026-08-19 | `RX_Master` & `TX_Node` | 1. ปรับเพิ่ม Turnaround Delay ฝั่ง Node เป็น `100ms` เพื่อให้ Master สลับเข้าโหมด RX สมบูรณ์ก่อนส่งตอบกลับ<br>2. ซิงค์ `LoRa.setSyncWord(0x12)` และ `LoRa.enableCrc()` ให้ตรงกันทั้งสองฝั่ง<br>3. ปรับ `POLL_TIMEOUT_MS = 3500ms` ให้ครอบคลุม Airtime ของ LoRa SF12<br>4. อัปเดตไฟล์ `water_flood.code-workspace` รวมทั้ง 3 ส่วนใน Workspace เดียว |
| 2026-08-19 | `RX_Master/src/main.cpp` | เปลี่ยนโหมด WiFi จาก Access Point (ปล่อยไวไฟ) เป็น **Station Mode (เชื่อมต่อ WiFi บ้าน/เราเตอร์)** พร้อมระบบ **mDNS (`http://floodmonitor.local`)** |

---

## 🏛️ สถาปัตยกรรมการทำงาน (System Architecture)

```
        [ WiFi Router / ไวไฟบ้าน ]
               |
               +-------------------------------------------------+
               |              ESP32-S3 (RX_Master)               |
               |  - เชื่อมต่อ WiFi บ้าน (Station Mode)           |
               |  - Web Server & mDNS (http://floodmonitor.local)|
               |  - LoRa SX1278 Polling Master (433MHz)          |
               |  - I2C OLED (SSD1306 128x64 Dashboard)          |
               +------------------------+------------------------+
                                        |
       +--------------------------------+-------------------------------+
       | (WiFi Web Browser: IP หรือ floodmonitor.local)                 | (LoRa 433MHz Polling)
       v                                                                v
[ 📱 Mobile Phone / PC ในวง WiFi เดียวกัน ]             +---------------+---------------+
(เปิดเบราว์เซอร์ดู Dashboard สด)                        |               |               |
                                                 +------v------+ +------v------+ +------v------+
                                                 |   Node 1    | |   Node 2    | |   Node 3    |
                                                 | (LoRa32u4)  | | (LoRa32u4)  | | (LoRa32u4)  |
                                                 | ID: 0x01    | | ID: 0x02    | | ID: 0x03    |
                                                 +-------------+ +-------------+ +-------------+
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

## 🚀 วิธีการอัปโหลดโค้ดเพื่อทดสอบ (Upload Guide)

### 1. อัปโหลดบอร์ด Node (LoRa32u4)
```bash
cd TX_Node
pio run -e node1 --target upload --upload-port COM22
```

### 2. อัปโหลดบอร์ด Master (ESP32-S3)
```bash
cd RX_Master
pio run --target upload --upload-port COM12
```

---

## 📱 วิธีการเข้าดูหน้าเว็บ Mobile Dashboard

1. เชื่อมต่อสมาร์ทโฟนหรือคอมพิวเตอร์เข้ากับ **WiFi วงเดียวกับที่ ESP32-S3 เชื่อมต่อ (CoE#01)**
2. ดู IP Address บนหน้าจอ OLED ของ ESP32-S3 (เช่น `192.168.1.xxx`)
3. เปิดเว็บเบราว์เซอร์แล้วพิมพ์:
   $$\text{\bf http://<IP_ของ_ESP32>} \quad \text{หรือ} \quad \text{\bf http://floodmonitor.local}$$
