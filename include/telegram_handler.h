#pragma once
#include <Arduino.h>
#include <WiFi.h>              
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

extern WiFiClientSecure client;
extern UniversalTelegramBot bot;

extern HardwareSerial mySerial;

// Все функции Telegram
void handleNewMessages(int numNewMessages);
void sendCommandList();
void sendClimateData();
void setTimeFromTelegram(String dateString);
void sendCurrentTime();
 void processCommand(char* command);
void readUartSTM();
void sendRP();
void RP_SendString(const char *ID, const char *string);
void RP_SendNum(const char *obj, int32_t num);
void RP_SendCommand(const char *string); 


