#include "telegram_handler.h"
#include "globals.h"
#include "sensors.h"
#include "uart_handler.h"
#include "ESP32UpDate.h"
#include "utils.h"  
#include <LittleFS.h>
#include <WiFiClientSecure.h> 

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

void sendClimateData() {
  updateBME280Data();
  String message;
  message.reserve(500);
  message += "Уличный климат:\n";
  message += "Температура: ";
  message += String(bmeT, 0);
  message += " °C\n";
  message += "Влажность: ";
  message += String(bmeH, 0);
  message += " %\n";
  message += "Давление: ";
  message += String(bmeP, 0);
  message += " mmHg\n";
  message += "******************\n";
  message += "Ванная комната:\n";
  message += "Уст. температура: ";
  message += String(settemp);
  message += " °C\n";
  message += "Температура: ";
  message += String(antT, 0);
  message += " °C\n";
  message += "Влажность: ";
  message += String(antH, 0);
  message += " %\n";
  message += "******************\n";
  message += "Комната отдыха:\n";
  message += "Уст. температура: ";
  message += String(settemp1);
  message += " °C\n";
  message += "Температура: ";
  message += String(bmeT1, 0);
  message += " °C\n";
  message += "Влажность: ";
  message += String(bmeH1, 0);
  message += " %\n";
  message += "******************\n";
  message += "Версия прошивки ESP: ";
  message += firmwareVersion;
  message += "\nВерсия прошивки STM32: ";
  message += stm32ver;
  message += "\n";
  bot.sendMessage(CHAT_ID, message, "");
  Serial.println("Данные климата отправлены.");
  message = "";
}

void sendCommandList() {
  String keyboardJson = "[[\"📅 /дата\", \"⏰ /часы\"],"
                       "[\"🛁 /ванная\", \"🏠 /комната\"],"
                       "[\"🌡 /климат\", \"🔄 /сброс\"],"
                       "[\"🔄 /обновить_ESP\", \"🔄 /обновить_STM\"]]";
  bot.sendMessageWithReplyKeyboard(CHAT_ID, "Выберите команду:", "", keyboardJson, true);
  Serial.println("Отправлено меню команд с кнопками");
}

void setTimeFromTelegram(String dateString) {
  if (!rtcFound) {
    bot.sendMessage(CHAT_ID, "RTC не найден. Установка времени невозможна.", "");
    Serial.println("RTC не найден. Установка времени невозможна.");
    waitingForDate = false;
    return;
  }
  if (dateString.length() == 12) {
    int day = dateString.substring(0, 2).toInt();
    int month = dateString.substring(2, 4).toInt();
    int year = dateString.substring(4, 6).toInt() + 2000;
    int hour = dateString.substring(6, 8).toInt();
    int minute = dateString.substring(8, 10).toInt();
    int second = dateString.substring(10, 12).toInt();
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    String response = "Время установлено: " + String(day) + "/" + String(month) + "/" + String(year) + " " + String(hour) + ":" + String(minute) + ":" + String(second);
    bot.sendMessage(CHAT_ID, response, "");
    Serial.println(response);
    waitingForDate = false;
  } else {
    bot.sendMessage(CHAT_ID, "Неверный формат даты. Используйте ДДММГГЧЧММСС", "");
    Serial.println("Получена неверная команда установки времени");
  }
}

void sendCurrentTime() {
  if (!rtcFound) {
    bot.sendMessage(CHAT_ID, "RTC не найден. Время недоступно.", "");
    Serial.println("RTC не найден. Время недоступно.");
    return;
  }
  DateTime now = rtc.now();
  String timeStr = String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
  bot.sendMessage(CHAT_ID, "Текущее время: " + timeStr, "");
  Serial.println("Отправлено текущее время: " + timeStr);
}

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(CHAT_ID);
    String text = bot.messages[i].text;
    Serial.print("Получено сообщение из Telegram: ");
    Serial.println(text);
    if (waitingForDate) {
      setTimeFromTelegram(text);
    }
    else if (text == "📅 /дата" || text == "/дата") {
      waitingForDate = true;
      bot.sendMessage(chat_id, "Отправьте дату и время в формате ДДММГГЧЧММСС", "");
    }
    else if (text == "⏰ /часы" || text == "/часы") {
      sendCurrentTime();
    }
    else if (text == "🛁 /ванная" || text == "/ванная") {
      bot.sendMessage(chat_id, "Установите температуру от 15 до 33", "");
      waitingForTemp = true;
    }
    else if (text == "🏠 /комната" || text == "/комната") {
      bot.sendMessage(chat_id, "Установите температуру от 15 до 33", "");
      waitingForTempRest = true;
    }
    else if (text == "🔄 /сброс" || text == "/сброс") {
       RP_SendCommand("rebootSTM32");
       Serial.println("Telegram: запрос на перезагрузку STM32");
       bot.sendMessage(chat_id, "🔄 Отправлен запрос на перезагрузку STM32...", "");
}
    else if (text == "rebootSTM") {
       Serial.println("Telegram:  перезагрузка STM32");
       bot.sendMessage(chat_id, "🔄  перезагрузка STM32...", "");
       digitalWrite(resetPin, LOW);
       delay(100);
       digitalWrite(resetPin, HIGH);
       bot.sendMessage(chat_id, "STM32 перезагружен", "");
    }
    else if (text == "/старт") {
      bot.sendMessage(chat_id, "Выберите комнду...", "");
      sendCommandList();
    }
    else if (waitingForTemp) {
      float temp = text.toFloat();
      if (temp >= 15 && temp <= 33) {
        settemp = temp;
        saveSettings();
        waitingForTemp = false;
        bot.sendMessage(chat_id, "Температура в ванной сохранена: " + String(temp), "");
        RP_SendNum("settemp", settemp);
      } else {
        bot.sendMessage(chat_id, "Неверное значение. Установите температуру от 15 до 33", "");
      }
    }
    else if (waitingForTempRest) {
      float temp = text.toFloat();
      if (temp >= 15 && temp <= 33) {
        settemp1 = temp;
        saveSettings();
        waitingForTempRest = false;
        bot.sendMessage(chat_id, "Температура в комнате сохранена: " + String(temp), "");
        RP_SendNum("settemp1", settemp1);
      } else {
        bot.sendMessage(chat_id, "Неверное значение. Установите температуру от 15 до 33", "");
      }
    }
    else if (text == "🌡 /климат" || text == "/климат") {
      sendClimateData();
    }
    else if (text == "🔄 /обновить_ESP" || text == "/обновить_ESP") {
      bot.sendMessage(chat_id, "Вы хотите обновить прошивку ESP? Ответьте 'да' или 'нет'", "");
      firmwareUpdateMode = true;
    }
    else if (text == "🔄 /обновить_STM" || text == "/обновить_STM") {
      bot.sendMessage(chat_id, "Вы хотите обновить прошивку STM32? Ответьте 'да' или 'нет'", "");
      firmwareUpdateSTMMode = true;
    }
    else if (text == "да" && firmwareUpdateMode) {
      enterFirmwareUpdateMode();
    }
    else if (text == "да" && firmwareUpdateSTMMode) {
      String url = "http://" + WiFi.localIP().toString() + "/";
      bot.sendMessage(chat_id, "STM32 будет переведён в режим загрузчика при загрузке прошивки.\nПерейдите по ссылке для загрузки прошивки:\n" + url, "");
    }
    else if (text == "нет" && firmwareUpdateMode) {
      exitFirmwareUpdateMode();
    }
    else if (text == "нет" && firmwareUpdateSTMMode) {
      firmwareUpdateSTMMode = false;
      bot.sendMessage(chat_id, "STM32 вышел из режима прошивки", "");
    }
    else if (text == "ок" && firmwareUpdateMode) {
      bot.sendMessage(chat_id, "Прошивка ESP успешно завершена. Версия прошивки: " + String(firmwareVersion), "");
      exitFirmwareUpdateMode();
    }
    else if (text == "ок" && firmwareUpdateSTMMode) {
      firmwareUpdateSTMMode = false;
      bot.sendMessage(chat_id, "Прошивка STM32 успешно завершена. Версия прошивки: " + String(firmwareVersion), "");
    }
  }
}
