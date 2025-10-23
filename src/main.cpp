#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <RTClib.h>
// #include <PubSubClient.h>
#include <WebServer.h>
#include <Update.h>
#include <LittleFS.h>
#include <FS.h>

// Подключаем все наши модули
#include "globals.h"
#include "sensors.h"
#include "utils.h"           // ДОБАВИТЬ ПЕРЕД uart_handler
#include "STM32UpDate.h"
#include "uart_handler.h"
#include "telegram_handler.h"
#include "ESP32UpDate.h"
#include "mqtt_handler.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Начало инициализации");
  delay(2000);

  // Инициализация LittleFS ПЕРВОЙ
  if (!LittleFS.begin()) {
    Serial.println("Ошибка монтирования LittleFS");
  } else {
    loadSettings();
  }

  // Удаляем старый файл прошивки STM32 если есть
  if (LittleFS.exists("/firmware.bin")) {
    LittleFS.remove("/firmware.bin");
    Serial.println("Удалён старый файл /firmware.bin");
  }

  // Настройка пинов управления UART
  pinMode(rx_conn, OUTPUT);
  pinMode(tx_conn, OUTPUT);
  pinMode(3, OUTPUT);
  digitalWrite(rx_conn, LOW);
  digitalWrite(tx_conn, LOW);
  Serial.println("UART отключен для инициализации");

  // Инициализация I2C
  Wire.begin(8, 9); // SDA, SCL
  Serial.println("I2C шина инициализирована");
  delay(100);

  setupWiFi();
  client.setInsecure();

  // Инициализация BME280
  if (!bme.begin(0x76)) {
    Serial.println("Не удалось найти BME280 по адресу 0x76");
  } else {
    Serial.println("BME280 успешно инициализирован");
  }

  // Инициализация RTC
  if (!rtc.begin()) {
    Serial.println("Не удалось найти RTC");
    rtcFound = false;
  } else {
    Serial.println("RTC успешно инициализирован");
    rtcFound = true;
  }

  // Включение UART
  Serial.println("Включение UART...");
  digitalWrite(rx_conn, HIGH);
  digitalWrite(tx_conn, HIGH);
  delay(100);
  mySerial.begin(115200, SERIAL_8N1, 18, 17);
  Serial.println("UART инициализирован и включен, скорость: 115200");

  pinMode(bootPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  digitalWrite(bootPin, LOW);
  digitalWrite(resetPin, HIGH);

  // Настройка MQTT (закомментирована)
  // setupMQTT();

  setupWebServer();

  bot.sendMessage(CHAT_ID, "Старт системы умного дома", "");
  delay(10);
  sendCommandList();

  RP_SendNum("settemp", settemp);
  RP_SendNum("settemp1", settemp1);
}

void loop() {
  // Проверка WiFi (неблокирующая)
  static unsigned long lastWifiCheck = 0;
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiCheck > 5000) {
      Serial.println("Переподключаем WiFi...");
      WiFi.begin(ssid, password);
      lastWifiCheck = millis();
    }
  }

  // Индикатор работы цикла
  
   static unsigned long lastLedToggle = 0;
  if (millis() - lastLedToggle >= 100) {
    lastLedToggle = millis();
    digitalWrite(3, !digitalRead(3));
  }

  // Обработка Telegram (оригинальная версия)
  if (millis() - bot_lasttime > 1000) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    if (numNewMessages > 0) {
      Serial.printf("Получено %d сообщений Telegram\n", numNewMessages);
      handleNewMessages(numNewMessages);
    }
    bot_lasttime = millis();
  }
  
  // Обмен данными с STM32 (оригинальная версия)
  if (!firmwareUpdateSTMMode) {
    if (millis() - lastSendRP > 2000) {
      sendRP();
      lastSendRP = millis();
    }
    readUartSTM();
    if (ackSettempValue != settemp && millis() - lastSettempSend >= 500) {
      RP_SendNum("settemp", settemp);
      lastSettempSend = millis();
      Serial.print("Повторная отправка settemp STM32: ");
      Serial.println(settemp);
    }
    if (ackSettemp1Value != settemp1 && millis() - lastSettemp1Send >= 500) {
      RP_SendNum("settemp1", settemp1);
      lastSettemp1Send = millis();
      Serial.print("Повторная отправка settemp1 STM32: ");
      Serial.println(settemp1);
    }
  }
  
  // Обработка MQTT (закомментирована)
  // handleMQTT();
  
  // Обработка веб-сервера
  httpServer.handleClient();
  
  // Проверка на перезагрузку
  if (rebootRequested && millis() >= rebootTime) {
    Serial.println("Выполняю перезагрузку...");
    rebootController();
  }
  
  // Проверка памяти каждые 5 секунд
  if (millis() - lastMemoryCheck > 5000) {
    printMemoryInfo();
    lastMemoryCheck = millis();
  }
    // Проверяем когда последний раз STM32 отзывался
  if (millis() - lastSTM32Response > 60000) { // 60 секунд = 60000 мс
    Serial.println("🔄 STM32 не отвежает больше минуты - аппаратный ресет!");
    bot.sendMessage(CHAT_ID, "STM32 не отвежает, делаю аппаратный ресет!", "");
        
        // Дергаем ресет STM32
    digitalWrite(resetPin, LOW);
    delay(100);
    digitalWrite(resetPin, HIGH);
  
        // Сбрасываем таймер чтобы не спамить
    lastSTM32Response = millis();
    bot.sendMessage(CHAT_ID, "STM32 перезагружен", ""); 
    Serial.println("✅ Ресет STM32 выполнен");
  }
}
