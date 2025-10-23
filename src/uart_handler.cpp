#include "uart_handler.h"
#include "globals.h"
#include "sensors.h"
#include "telegram_handler.h"
#include "utils.h"  
#include <LittleFS.h>

//==============Настройки UART для обмена данными с Nextion=====================//
HardwareSerial mySerial(1); // UART1 для ESP32

const uint8_t RP_End[2] = {0x0D, 0x0A};

void RP_SendString(const char *ID, const char *string) {
  int len = snprintf(tempStrBuffer, sizeof(tempStrBuffer), "%s.txt=\"%s\"", ID, string);
  if (len < sizeof(tempStrBuffer)) {
    mySerial.write((const uint8_t *)tempStrBuffer, len);
    mySerial.write(RP_End, 2);
  }
}

void RP_SendNum(const char *obj, int32_t num) {
  int len = snprintf(tempStrBuffer, sizeof(tempStrBuffer), "%s=%ld", obj, (long)num);
  if (len < sizeof(tempStrBuffer)) {
    mySerial.write((const uint8_t *)tempStrBuffer, len);
    mySerial.write(RP_End, 2);
  }
}

void RP_SendCommand(const char *string) {
    // Вычисляем длину строки
    size_t str_len = strlen(string);
    size_t max_len = sizeof(tempStrBuffer) - 1;
    
    // Определяем фактическую длину для копирования
    size_t copy_len = (str_len < max_len) ? str_len : max_len;
    
    // Копируем данные
    memcpy(tempStrBuffer, string, copy_len);
    tempStrBuffer[copy_len] = '\0'; // Добавляем нулевой терминатор
    
    // Отправляем
    mySerial.write((const uint8_t *)tempStrBuffer, copy_len);
    mySerial.write(RP_End, 2);
}

void sendRP() {
  updateBME280Data();
  RP_SendNum("bmeT", (int32_t)bmeT);
  RP_SendNum("bmeH", (int32_t)bmeH);
  RP_SendNum("bmeP", (int32_t)bmeP);
}

void processCommand(char* command) {
  Serial.print("Буфер : ");
  Serial.println(command);
  if (strncmp(command, "antT=", 5) == 0) antT = atof(command + 5);
  else if (strncmp(command, "antH=", 5) == 0) antH = atof(command + 5);
  else if (strncmp(command, "bmeT1=", 6) == 0) bmeT1 = atof(command + 6);
  else if (strncmp(command, "bmeH1=", 6) == 0) bmeH1 = atof(command + 6);
  else if (strncmp(command, "bmeP1=", 6) == 0) bmeP1 = atof(command + 6);
  else if (strncmp(command, "rebootSTM32_ok", 14) == 0) {
    Serial.println("✅ STM32 подтвердил перезагрузку");
  
       // Отправляем сообщение в Telegram
    bot.sendMessage(CHAT_ID, "✅ STM32 успешно перезагружен. ESP32 перезагрузится через 3 секунды...", "");
  
       // Устанавливаем флаг и время ребута для ESP32
    rebootRequested = true;
    rebootTime = millis() + 3000; // Ребута через 3 секунды
  
  Serial.println("🔄 ESP32 перезагрузится через 3 секунды");
}
  else if (strncmp(command, "STM32_Status_ok", 15) == 0) {
    lastSTM32Response = millis(); // Обновляем время последнего ответа
    Serial.println("✅ STM32 подтвердил работу");
}
  else if (strncmp(command, "errorAnt20", 10) == 0) {
    Serial.println("Не корректные данные с Ant20");
    bot.sendMessage(CHAT_ID, "Не валидные данные с климатического датчика в ванной комнате", "");
}
  else if (strncmp(command, "errorBME280", 11) == 0) {
    Serial.println("Не корректные данные с BME280");
    bot.sendMessage(CHAT_ID, "Не валидные данные с климатического датчика в комнате отдыха", "");
}
  else if (strncmp(command, "settemp=", 8) == 0) {
  uint8_t newSettemp = atoi(command + 8);
  if (settemp != newSettemp) {
    settemp = newSettemp;
    saveSettings();
    Serial.print("Буфер : settemp изменён STM32 -> ESP: ");
    Serial.println(settemp);
    RP_SendNum("settemp_ok", settemp);
    Serial.print("Подтверждение :settemp_ok=");
    Serial.println(settemp);
    
    // ✅ СООБЩЕНИЕ ПРИ ИЗМЕНЕНИИ С ДИСПЛЕЯ
    int len = snprintf(tempStrBuffer, sizeof(tempStrBuffer),
                      "🎛️ С ДИСПЛЕЯ: Ванная температура %d °C\n🌡 Текущая: %.0f °C",
                      settemp, antT);
    if (len < sizeof(tempStrBuffer)) {
      bot.sendMessage(CHAT_ID, tempStrBuffer, "");
    }
  }
  ackSettempValue = settemp;
}
  else if (strncmp(command, "settemp1=", 9) == 0) {
  uint8_t newSettemp1 = atoi(command + 9);
  if (settemp1 != newSettemp1) {
    settemp1 = newSettemp1;
    saveSettings();
    Serial.print("Буфер : settemp1 изменён STM32 -> ESP: ");
    Serial.println(settemp1);
    RP_SendNum("settemp1_ok", settemp1);
    Serial.print("Подтверждение :settemp1_ok=");
    Serial.println(settemp1);
    
    // ✅ СООБЩЕНИЕ ПРИ ИЗМЕНЕНИИ С ДИСПЛЕЯ
    int len = snprintf(tempStrBuffer, sizeof(tempStrBuffer),
                      "🎛️ С ДИСПЛЕЯ: Комната температура %d °C\n🌡 Текущая: %.0f °C",
                      settemp1, bmeT1);
    if (len < sizeof(tempStrBuffer)) {
      bot.sendMessage(CHAT_ID, tempStrBuffer, "");
    }
  }
  ackSettemp1Value = settemp1;
}
  else if (strncmp(command, "settemp_st_ok=", 14) == 0) {
    int v = atoi(command + 14);
    ackSettempValue = v;
    if (settemp != v) {
      settemp = v;
      saveSettings();
    }
    Serial.print("Подтверждение от STM32 :settemp_st_ok=");
    Serial.println(v);
    int len = snprintf(tempStrBuffer, sizeof(tempStrBuffer),
                      "Температура в ванной комнате подтверждена STM32: %d °C, текущая температура: %.0f °C",
                      v, antT);
    if (len < sizeof(tempStrBuffer)) {
      bot.sendMessage(CHAT_ID, tempStrBuffer, "");
    }
  }
  else if (strncmp(command, "settemp1_st_ok=", 15) == 0) {
    int v = atoi(command + 15);
    ackSettemp1Value = v;
    if (settemp1 != v) {
      settemp1 = v;
      saveSettings();
    }

    Serial.print("Подтверждение от STM32 :settemp1_st_ok=");
    Serial.println(v);
    int len = snprintf(tempStrBuffer, sizeof(tempStrBuffer),
                      "Температура в комнате отдыха подтверждена STM32: %d °C, текущая температура: %.0f °C",
                      v, bmeT1);
    if (len < sizeof(tempStrBuffer)) {
      bot.sendMessage(CHAT_ID, tempStrBuffer, "");
    }
  }
  else if (strncmp(command, "stm32ver.txt=", 13) == 0) {
    stm32ver = command + 13;
    Serial.print("Версия прошивки STM32: ");
    Serial.println(stm32ver);
  }
}

void readUartSTM() {
  static char localBuffer[256];
  static uint8_t localIndex = 0;
  
  while (mySerial.available()) {
    char c = mySerial.read();
    
    if (c == '\n' || c == '\r') {
      if (localIndex > 0) {
        localBuffer[localIndex] = '\0';
        
        // ✅ ЧИСТАЯ ОБРАБОТКА - без костылей
        Serial.printf("📨 UART received: %s\n", localBuffer); // для отладки
        
        char *command = strtok(localBuffer, "\r\n");
        while (command != NULL) {
          processCommand(command);
          command = strtok(NULL, "\r\n");
        }
        
        localIndex = 0;
      }
    } else if (localIndex < sizeof(localBuffer) - 1) {
      localBuffer[localIndex++] = c;
    } else {
      // Буфер переполнен - сбрасываем
      Serial.println("⚠️ Буфер ЮАРТ переполнен...Сброс");
      localIndex = 0;
    }
  }
}

