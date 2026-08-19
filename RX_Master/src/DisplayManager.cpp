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

    // --- Header ---
    display.fillRect(0, 0, SCREEN_WIDTH, 11, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setTextSize(1);
    display.setCursor(2, 2);
    display.print(F("Flood Master RTOS"));

    const char animChar[] = {'-', '\\', '|', '/'};
    display.setCursor(110, 2);
    display.printf("%c#%d", animChar[animStep % 4], pollCycleCount % 100);

    // --- IP Line ---
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 13);
    if (WiFi.status() == WL_CONNECTED) {
      display.print(F("IP:"));
      display.println(WiFi.localIP());
    } else {
      display.println(F("WiFi: Connecting..."));
    }

    // --- Node Rows ---
    const char* statusShort[] = {"OK", "WARN", "ALERT"};

    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
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
      xSemaphoreGive(dataMutex);
    }

    display.display();
    vTaskDelay(pdMS_TO_TICKS(400));
  }
}
