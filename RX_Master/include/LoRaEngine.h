#ifndef LORA_ENGINE_H
#define LORA_ENGINE_H

#include <Arduino.h>

// FreeRTOS Task สำหรับ LoRa Polling Engine
void TaskLoRaPolling(void *pvParameters);

#endif // LORA_ENGINE_H
