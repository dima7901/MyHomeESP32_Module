#include "utils.h"
#include "globals.h"
#include <LittleFS.h>
#include <WiFi.h>
#include "telegram_handler.h"

void scheduleReboot() {
  rebootRequested = true;
  rebootTime = millis() + 3000;
}

bool shouldReboot() {
  return rebootRequested && millis() >= rebootTime;
}

void printMemoryInfo() {
  Serial.printf("RAM: %d free\n", esp_get_free_heap_size());
}

bool saveSettings() {
  File file = LittleFS.open("/settings.txt", "w");
  if (!file) return false;
  file.println(settemp);
  file.println(settemp1);
  file.close();
  Serial.printf("Сохранены настройки: settemp=%d, settemp1=%d\n", settemp, settemp1);
  return true;
}

bool loadSettings() {
  File file = LittleFS.open("/settings.txt", "r");
  if (!file) return false;
  settemp = file.readStringUntil('\n').toInt();
  settemp1 = file.readStringUntil('\n').toInt();
  file.close();
  Serial.printf("Загружены настройки: settemp=%d, settemp1=%d\n", settemp, settemp1);
  return true;
}

void rebootController() {
  Serial.println("Перезагрузка контроллера...");
  delay(1000);
  ESP.restart();
}

void setupWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Подключение к WiFi");
  WiFi.begin(ssid, password);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWiFi Подключен. IP адрес: ");
    Serial.println(WiFi.localIP());
    bot.sendMessage(CHAT_ID, "Умный дом подключен к Wi-Fi", "");
  } else {
    Serial.println("\nОшибка подключения к WiFi");
  }
  delay(100);
}
