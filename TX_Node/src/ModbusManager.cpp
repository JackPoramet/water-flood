#include "ModbusManager.h"

#ifdef USE_MODBUS_SENSOR

// =========================================================================
// ModbusManager — จัดการ RS485 Modbus RTU สำหรับเซนเซอร์ DJLK-003AB
// =========================================================================
// ฮาร์ดแวร์:
//   Serial1 (Hardware UART) : RX=D0, TX=D1
//   MAX485 Transceiver      : RE=D6 (Active LOW), DE=D9 (Active HIGH)
//
// Modbus Request Frame: 01 03 01 00 00 01 85 F6
//   → Slave ID=1, Func=03 (Read Holding Registers)
//   → Register Address=0x0100, Quantity=1
//   → Response: ระยะทาง (mm), Unsigned 16-bit
// =========================================================================

#include <ModbusMaster.h>

static ModbusMaster modbusNode;
static bool sensorOnline = false;

// -------------------------------------------------------------------------
// Callback สลับ MAX485 เข้าโหมดส่ง (Transmit Mode)
// -------------------------------------------------------------------------
static void preTransmission() {
  digitalWrite(PIN_RS485_RE, HIGH);   // ปิดตัวรับ (RE = Active LOW)
  digitalWrite(PIN_RS485_DE, HIGH);   // เปิดตัวส่ง (DE = Active HIGH)
}

// -------------------------------------------------------------------------
// Callback สลับ MAX485 กลับเข้าโหมดรับ (Receive Mode)
// -------------------------------------------------------------------------
static void postTransmission() {
  // หน่วงให้ byte สุดท้ายส่งออกทาง bus เสร็จสมบูรณ์
  delayMicroseconds(650);
  digitalWrite(PIN_RS485_RE, LOW);    // เปิดตัวรับ
  digitalWrite(PIN_RS485_DE, LOW);    // ปิดตัวส่ง
  // เคลียร์ echo bytes ที่ TX วนกลับเข้า RX buffer ออกให้หมด
  while (Serial1.available()) {
    Serial1.read();
  }
}

// =========================================================================
// Public Functions
// =========================================================================

void initModbus() {
  // ตั้งค่า DE/RE pins สำหรับ MAX485
  pinMode(PIN_RS485_RE, OUTPUT);
  pinMode(PIN_RS485_DE, OUTPUT);

  // เริ่มต้นในโหมดรับ (RE=LOW, DE=LOW)
  digitalWrite(PIN_RS485_RE, LOW);
  digitalWrite(PIN_RS485_DE, LOW);

  // เริ่มต้น Serial1 (Hardware UART: RX=D0, TX=D1)
  Serial1.begin(MODBUS_BAUD_RATE);

  // เริ่มต้น ModbusMaster กับ Slave ID ของ DJLK-003AB
  modbusNode.begin(MODBUS_SLAVE_ID, Serial1);
  modbusNode.preTransmission(preTransmission);
  modbusNode.postTransmission(postTransmission);

  Serial.println(F("[+] Modbus RTU Initialized (Serial1, 9600 8N1)"));
  Serial.print(F("[+] DJLK-003AB Slave ID: "));
  Serial.print(MODBUS_SLAVE_ID);
  Serial.print(F(", Register: 0x"));
  Serial.println(MODBUS_REG_ADDR, HEX);
}

bool readModbusWaterLevel(uint16_t &waterLevelMm) {
  // อ่าน 1 Holding Register จาก Address 0x0100 (retry สูงสุด 3 ครั้ง)
  const uint8_t MAX_RETRIES = 3;

  for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++) {
    uint8_t result = modbusNode.readHoldingRegisters(MODBUS_REG_ADDR, 1);

    if (result == modbusNode.ku8MBSuccess) {
      waterLevelMm = modbusNode.getResponseBuffer(0);
      sensorOnline = true;

      Serial.print(F("[Modbus] OK: "));
      Serial.print(waterLevelMm);
      Serial.print(F("mm ("));
      Serial.print(waterLevelMm / 10);
      Serial.println(F("cm)"));

      return true;
    }

    // Retry — หน่วง 50ms ให้เซนเซอร์พร้อม
    if (attempt < MAX_RETRIES - 1) {
      delay(50);
    } else {
      // ครบ retry แล้วยังไม่สำเร็จ
      sensorOnline = false;
      Serial.print(F("[Modbus] FAIL after "));
      Serial.print(MAX_RETRIES);
      Serial.print(F(" retries (last err: 0x"));
      Serial.print(result, HEX);
      Serial.println(F(")"));
    }
  }

  return false;
}

bool isModbusSensorOnline() {
  return sensorOnline;
}

#endif // USE_MODBUS_SENSOR
