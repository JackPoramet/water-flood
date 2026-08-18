#ifndef THAI_FONT_H
#define THAI_FONT_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

// =========================================================================
// Thai Font Engine for Adafruit_GFX (8x16 Bitmap Font with Vowel Stacking)
// รองรับภาษาไทย UTF-8 สมบูรณ์แบบ สระบน/ล่าง/วรรณยุกต์ ไม่ทับซ้อน
// =========================================================================

// ตารางฟอนต์ไทยและอังกฤษ 8x16 (CP874 / TIS-620)
extern const unsigned char thai_font_8x16[] PROGMEM;

// ฟังก์ชันแปลง UTF-8 ภาษาไทยเป็นรหัส TIS-620
inline uint8_t utf8_to_tis620(const char*& p) {
  uint8_t c = (uint8_t)*p++;
  if (c < 0x80) {
    return c; // ASCII ปกติ
  } else if (c == 0xE0) {
    uint8_t c2 = (uint8_t)*p++;
    uint8_t c3 = (uint8_t)*p++;
    if (c2 == 0xB8) {
      return c3 + 0x20; // 0xA0 - 0xDF
    } else if (c2 == 0xB9) {
      return c3 + 0x60; // 0xE0 - 0xFB
    }
  }
  return ' ';
}

// ตรวจสอบระดับของสระและวรรณยุกต์
// 0 = พยัญชนะปกติ
// 1 = สระบน (ิ ี ึ ื ั ็)
// 2 = วรรณยุกต์บน (่ ้ ๊ ๋ ์ ํ)
// -1 = สระล่าง (ุ ู ฺ)
// 3 = สระอำ (ำ)
inline int getThaiLevel(uint8_t c) {
  // สระบน Level 1
  if (c == 0xD1 || (c >= 0xD4 && c <= 0xD7) || c == 0xE7) return 1;
  // วรรณยุกต์ Level 2
  if (c >= 0xE8 && c <= 0xEC) return 2;
  // สระล่าง Level -1
  if (c >= 0xD8 && c <= 0xDA) return -1;
  // สระอำ
  if (c == 0xD3) return 3;
  return 0;
}

// ฟังก์ชันวาดตัวอักษร 8x16 ลงจอ Adafruit_GFX
inline void drawThaiChar(Adafruit_GFX &gfx, int16_t x, int16_t y, uint8_t code, uint16_t color, uint16_t bg, uint8_t size, int yOffset = 0) {
  if (code < 0x20) return;
  
  // Offset ในตาราง Font
  int glyphIndex = code - 0x20;
  const unsigned char* glyph = thai_font_8x16 + (glyphIndex * 16);

  for (int row = 0; row < 16; row++) {
    uint8_t line = pgm_read_byte(glyph + row);
    for (int col = 0; col < 8; col++) {
      if (line & (0x80 >> col)) {
        if (size == 1) {
          gfx.drawPixel(x + col, y + row + yOffset, color);
        } else {
          gfx.fillRect(x + (col * size), y + ((row + yOffset) * size), size, size, color);
        }
      } else if (bg != color && bg != 0) {
        if (size == 1) {
          gfx.drawPixel(x + col, y + row + yOffset, bg);
        } else {
          gfx.fillRect(x + (col * size), y + ((row + yOffset) * size), size, size, bg);
        }
      }
    }
  }
}

// ฟังก์ชันหลัก: พิมพ์ข้อความภาษาไทยและอังกฤษได้ทุกรูปแบบ
inline void drawThaiText(Adafruit_GFX &gfx, const char* text, int16_t x, int16_t y, uint16_t color = 1, uint8_t size = 1) {
  int16_t curX = x;
  int16_t lastBaseX = x;
  bool hasTopVowel = false;

  const char* p = text;
  while (*p) {
    uint8_t tis = utf8_to_tis620(p);
    int level = getThaiLevel(tis);

    if (level == 0) {
      // พยัญชนะ หรือ สระหน้า/หลัง (ก-ฮ, เ, แ, โ, ใ, ไ, ะ, า, ๆ, ฯ)
      drawThaiChar(gfx, curX, y, tis, color, 0, size, 0);
      lastBaseX = curX;
      curX += 8 * size;
      hasTopVowel = false;
    } else if (level == 1) {
      // สระบน (ิ ี ึ ื ั ็) ซ้อนบนพยัญชนะตัวก่อนหน้า
      drawThaiChar(gfx, lastBaseX, y, tis, color, 0, size, -2);
      hasTopVowel = true;
    } else if (level == 2) {
      // วรรณยุกต์ (่ ้ ๊ ๋ ์) ซ้อนบนพยัญชนะ (ขยับขึ้นสูงขึ้นหากมีสระบนอยู่ก่อนแล้ว)
      int offset = hasTopVowel ? -6 : -2;
      drawThaiChar(gfx, lastBaseX, y, tis, color, 0, size, offset);
    } else if (level == -1) {
      // สระล่าง (ุ ู) ซ้อนใต้พยัญชนะ
      drawThaiChar(gfx, lastBaseX, y, tis, color, 0, size, 2);
    } else if (level == 3) {
      // สระอำ (ำ) = นิคหิตบน + สระอา
      drawThaiChar(gfx, lastBaseX, y, 0xED, color, 0, size, -2);
      drawThaiChar(gfx, curX, y, 0xD2, color, 0, size, 0);
      lastBaseX = curX;
      curX += 8 * size;
      hasTopVowel = false;
    }
  }
}

#endif // THAI_FONT_H
