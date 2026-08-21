#include "ModbusManager.h"

#ifdef USE_MODBUS_SENSOR

// =========================================================================
// ModbusManager — จัดการ RS485 Modbus RTU สำหรับเซนเซอร์ DJLK-003AB
// =========================================================================
// ฮาร์ดแวร์:
//   Serial1 (Hardware UART) : RX=D0, TX=D1
//   MAX485 Transceiver      : RE=D6 (Active LOW), DE=D9 (Active HIGH)
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
  delayMicroseconds(650);
  digitalWrite(PIN_RS485_RE, LOW);    // เปิดตัวรับ
  digitalWrite(PIN_RS485_DE, LOW);    // ปิดตัวส่ง
  while (Serial1.available()) {
    Serial1.read();
  }
}

// =========================================================================
// Public Functions
// =========================================================================

void initModbus() {
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
  Serial.print(F(", Primary Register: 0x"));
  Serial.println(MODBUS_REG_ADDR, HEX);
}

bool readModbusWaterLevel(uint16_t &waterLevelMm) {
  // 1. ลองอ่าน Register 0x0100 (Processed value)
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

  // 2. ถ้าไม่สำเร็จ ลองอ่าน Register 0x0101 (Real-time value ตอบสนองไว 100ms)
  delay(80);
  result = modbusNode.readHoldingRegisters(0x0101, 1);

  if (result == modbusNode.ku8MBSuccess) {
    waterLevelMm = modbusNode.getResponseBuffer(0);
    sensorOnline = true;

    Serial.print(F("[Modbus] OK (Realtime): "));
    Serial.print(waterLevelMm);
    Serial.print(F("mm ("));
    Serial.print(waterLevelMm / 10);
    Serial.println(F("cm)"));

    return true;
  }

  sensorOnline = false;
  return false;
}

bool isModbusSensorOnline() {
  return sensorOnline;
}

#endif // USE_MODBUS_SENSOR
