#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>

// FreeRTOS Task สำหรับ Web Server & WiFi
void TaskWebServer(void *pvParameters);

#endif // WEB_PORTAL_H
