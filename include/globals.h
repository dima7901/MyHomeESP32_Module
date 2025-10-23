#pragma once
#include <Arduino.h>

//==============Глобальные переменные и настройки=================//

// Пины
extern const int rx_conn;
extern const int tx_conn;
extern const int bootPin;
extern const int resetPin;

// Данные датчиков
extern bool waterFlowFlag;
extern float totalLiters, flowRateMinute, flowRate;
extern float antT, antH, bmeT, bmeH, bmeP, bmeT1, bmeH1, bmeP1;

// Настройки температур
extern uint8_t settemp;
extern uint8_t settemp1;
extern uint8_t settemp_ok, settemp1_ok;

// Режимы прошивки
extern bool firmwareUpdateMode;
extern bool firmwareUpdateSTMMode;

// Чек активности цикла СТМ32

extern unsigned long lastSTM32Response;

// Версии
extern const char* firmwareVersion;
extern String stm32ver;

// Настройки WiFi
extern const char* ssid;
extern const char* password;

// Настройки Telegram
extern const char* CHAT_ID;
extern const char* BOTtoken;

// Настройки MQTT (закомментированы)
// extern const char* mqtt_server;
// extern const int mqttPort;
// extern const char* mqttUser;
// extern const char* mqttPassword;

// Подтверждения температур
extern int ackSettempValue;
extern int ackSettemp1Value;
extern unsigned long lastSettempSend;
extern unsigned long lastSettemp1Send;

// Интервалы отчетов
extern int reportInterval;

// Состояния ожидания ввода
extern bool waitingForDate;
extern bool waitingForTemp;
extern bool waitingForTempRest;

// UART буферы
extern const int BUFFER_SIZE;
extern char buffer[256];
extern int bufferIndex;
extern unsigned long uartBaud;
extern char tempStrBuffer[300];
extern char climateMsgBuffer[600];
extern char timeStrBuffer[60];
extern char logStrBuffer[300];

// Таймеры
extern unsigned long bot_lasttime;
extern unsigned long lastSendRP;
extern unsigned long lastUartRead;
extern unsigned long lastMemoryCheck;
extern bool rebootRequested;
extern unsigned long rebootTime;

// Строка причины перезагрузки
extern String resetReason;
