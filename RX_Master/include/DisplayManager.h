#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

// FreeRTOS Task สำหรับหน้าจอ OLED SSD1306
void TaskOLEDDisplay(void *pvParameters);

#endif // DISPLAY_MANAGER_H
