#include "globals.h"

//==============Инициализация глобальных переменных===============//

// Пины
const int rx_conn = 33;
const int tx_conn = 21;
const int bootPin = 37;
const int resetPin = 39;

// Данные датчиков
bool waterFlowFlag = false;
float totalLiters = 0, flowRateMinute = 0, flowRate = 0;
float antT = 0, antH = 0, bmeT = 0, bmeH = 0, bmeP = 0, bmeT1 = 0, bmeH1 = 0, bmeP1 = 0;

// Настройки температур
uint8_t settemp, settemp1;
uint8_t settemp_ok, settemp1_ok;

// Режимы прошивки
bool firmwareUpdateMode = false;
bool firmwareUpdateSTMMode = false;

unsigned long lastSTM32Response = 0;

// Версии
const char* firmwareVersion = "1.0.1";
String stm32ver = "-";

   // Обьявы для сброса
bool rebootRequested = false;
unsigned long rebootTime = 0;

// Настройки WiFi
const char* ssid = "ZyXEL_KEENETIC_OMNI_417F54";
const char* password = "798647dg";

// Настройки Telegram
const char* CHAT_ID = "1193037735";
const char* BOTtoken = "6739890227:AAGTqBVnyxPwZyYLe2eRELVeswfzKb3q08c";

// Настройки MQTT (закомментированы)
// const char* mqtt_server = "m2.wqtt.ru";
// const int mqttPort = 13623;
// const char* mqttUser = "u_W97JJ6";
// const char* mqttPassword = "2dgQ6I1I";

// Подтверждения температур
int ackSettempValue = -1;
int ackSettemp1Value = -1;
unsigned long lastSettempSend = 0;
unsigned long lastSettemp1Send = 0;
// Интервалы отчетов
int reportInterval = 7;

// Состояния ожидания ввода
bool waitingForDate = false;
bool waitingForTemp = false;
bool waitingForTempRest = false;


// UART буферы
const int BUFFER_SIZE = 256;
char buffer[256] = {0};
int bufferIndex = 0;
unsigned long uartBaud = 115200;
char tempStrBuffer[300] = {0};
char climateMsgBuffer[600] = {0};
char timeStrBuffer[60] = {0};
char logStrBuffer[300] = {0};

// Таймеры
unsigned long bot_lasttime = 0;
unsigned long lastSendRP = 0;
unsigned long lastUartRead = 0;
unsigned long lastMemoryCheck = 0;

// Строка причины перезагрузки
String resetReason = "ESP32_START";
