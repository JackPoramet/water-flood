#include "DisplayManager.h"
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Config.h"
#include "SystemState.h"

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void TaskOLEDDisplay(void *pvParameters) {
  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  for (;;) {
    animStep++;
    display.clearDisplay();

    // --- Header (Yellow Zone: y = 0..14) ---
    display.fillRect(0, 0, SCREEN_WIDTH, 14, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setTextSize(1);
    display.setCursor(2, 3);
    display.print(F("Flood Master RTOS"));

    const char animChar[] = {'-', '\\', '|', '/'};
    display.setCursor(106, 3);
    display.printf("%c#%d", animChar[animStep % 4], pollCycleCount % 100);

    // --- Blue Zone (y = 18..63) ---
    display.setTextColor(SSD1306_WHITE);

    // Row 1: IP Address (y = 19..26 อยู่ในแถบสีน้ำเงินเต็มตัว ไม่ทับรอยต่อสี)
    display.setCursor(0, 19);
    if (WiFi.status() == WL_CONNECTED) {
      display.print(F("IP:"));
      display.println(WiFi.localIP());
    } else {
      display.println(F("WiFi: Connecting..."));
    }

    // Row 2 & 3: Node Rows (Node 1 ที่ y=32, Node 2 ที่ y=44)
    const char* statusShort[] = {"OK", "WARN", "ALERT"};

    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      for (int i = 0; i < PROTOCOL_MAX_NODES; i++) {
        int y = 32 + (i * 12);
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

      // Row 4: Status Footer (y = 56)
      display.setCursor(0, 56);
      display.printf("TG:%s | LoRa SF%d", (TELEGRAM_ENABLED ? "ON" : "OFF"), LORA_SF);

      xSemaphoreGive(dataMutex);
    }

    display.display();
    vTaskDelay(pdMS_TO_TICKS(400));
  }
}
