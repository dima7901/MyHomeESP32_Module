#pragma once
#include <Arduino.h>

extern HardwareSerial mySerial;

// Все функции UART и обработки команд
void processCommand(char* command);
void readUartSTM();
void sendRP();
void RP_SendString(const char *ID, const char *string);
void RP_SendNum(const char *obj, int32_t num);
void RP_SendCommand(const char *string);
