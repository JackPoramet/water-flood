#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <Arduino.h>
#include "Protocol.h"

// โครงสร้างข้อมูลสถานะของแต่ละ Node
struct NodeInfo {
  uint8_t id;
  const char* name;
  bool online;
  uint16_t waterLevelCm;
  uint8_t waterPercent;
  uint8_t floodStatus;
  uint16_t batteryMilliVolt;
  uint16_t uptimeSec;
  int rssi;
  float snr;
  unsigned long lastSeenMillis;
  uint32_t packetsReceived;
  uint32_t timeoutCount;
};

// สถานะการทำงานของ Polling State Machine
enum PollState {
  STATE_SEND_POLL,
  STATE_WAIT_RESPONSE,
  STATE_ADVANCE_NODE,
  STATE_CYCLE_WAIT
};

// ข้อมูลสถานะระบบส่วนกลาง (Shared Variables)
extern SemaphoreHandle_t dataMutex;
extern NodeInfo nodes[PROTOCOL_MAX_NODES];
extern PollState currentPollState;
extern uint8_t currentNodeIndex;
extern uint32_t pollCycleCount;
extern uint8_t animStep;
extern bool triggerManualPoll;

#endif // SYSTEM_STATE_H
