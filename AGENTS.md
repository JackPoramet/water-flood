# AGENTS.md - Project Rules & Development Guidelines

เอกสารข้อกำหนดและแนวทางการพัฒนาสำหรับ AI Agent และนักพัฒนาในโครงการ **Water Flood Monitoring System (LoRa Multi-Node + ESP32-S3 Web Dashboard)**

---

## 📌 1. กฎเหล็กด้านการบันทึกเอกสาร (Documentation Rule - MANDATORY)

> [!IMPORTANT]
> **ทุกครั้งที่มีการแก้ไข เพิ่มเติม หรือปรับปรุงโค้ดในโครงการนี้:**
> 1. **ต้องเขียน/อัปเดตเอกสารประกอบ (Documentation) เสมอ** ในไฟล์ `README.md` หรือเอกสารที่เกี่ยวข้อง
> 2. **ต้องจัดรูปแบบให้อ่านและเข้าใจง่าย:** ใช้หัวข้อชัดเจน, มีตารางสรุป Pinout, มีแผนภาพ ASCII/Mermaid, และใช้ภาษาไทยที่กระชับ ชัดเจน
> 3. **ต้องระบุรายการสิ่งที่เปลี่ยนแปลง (Changelog):** บันทึกว่าไฟล์ใดถูกแก้ไข เปลี่ยนแปลงอะไร และมีผลกระทบอย่างไร

---

## 📐 2. มาตรฐานการพัฒนาซอฟต์แวร์ (Coding Standards)

1. **การตั้งชื่อและโครงสร้างโปรเจกต์:**
   - ใช้ภาษา C++ (Arduino Framework บน PlatformIO)
   - ตัวแปรและฟังก์ชันใช้แบบ `camelCase` (เช่น `waterLevelCm`, `handlePollRequest`)
   - ค่าคงที่และ Macro Pin ใช้แบบ `UPPER_SNAKE_CASE` (เช่น `LORA_BAND`, `OLED_SDA`)

2. **โปรโตคอลการสื่อสาร (LoRa Communication):**
   - การสื่อสารระดับ Radio ต้องใช้โครงสร้าง Binary Frame ที่กำหนดไว้ใน `Protocol.h` เพื่อประหยัด Airtime และแบนด์วิดท์
   - ต้องมี Checksum / CRC เสมอ เพื่อตรวจสอบความสมบูรณ์ของข้อมูลก่อนนำไปประมวลผล
   - ระบบ LoRa Multi-Node ต้องใช้ระบบ **Master-Initiated Polling** ป้องกันสัญญาณชนกัน (Collision Avoidance)

3. **Web Server & Mobile Interface:**
   - หน้าเว็บต้องเป็น Responsive Design รองรับการแสดงผลบนสมาร์ทโฟนและคอมพิวเตอร์
   - การดึงข้อมูลต้องเป็น Asynchronous (Fetch API / JSON) โดยไม่ต้อง Reload หน้าเว็บ
   - ดีไซน์ต้องมีความทันสมัย มีสีสันระบุสถานะชัดเจน (เขียว = ปกติ, เหลือง = เฝ้าระวัง, แดง = วิกฤตน้ำท่วม)

---

## 🔌 3. แผนผังการต่อฮาร์ดแวร์มาตรฐาน (Hardware Pinout Reference)

### ฝั่ง Master: ESP32-S3
- **I2C OLED (SSD1306 128x64):** `SDA = GPIO 5`, `SCL = GPIO 4`
- **LoRa SX1278 (RA-02 433MHz):** 
  - `SCK = GPIO 12`, `MISO = GPIO 13`, `MOSI = GPIO 11`, `NSS/CS = GPIO 10`
  - `RST = จั๊มไฟ 3.3V`, `DIO0 = ไม่ต้องต่อ`
  - `VCC = 3.3V`, `GND = GND`

### ฝั่ง Node: LoRa32u4 (ATmega32U4)
- **LoRa SX1278 On-board:** `NSS/CS = D8`, `RST = D4`, `DIO0 = D7`, `LED = D13`
