#include "ESP32UpDate.h"
#include "globals.h"
#include "STM32UpDate.h"
#include "telegram_handler.h"
#include "uart_handler.h" 

WebServer httpServer(80);

void handleRoot() {
  uint32_t flashSize = ESP.getFlashChipSize();
  String html = "<html><head>"
                "<meta charset='UTF-8'>"
                "<style>"
                "body{font-family:Arial,sans-serif;background:#f0f8ff;margin:0;padding:20px}"
                ".container{max-width:600px;margin:auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}"
                "h1{color:#2c3e50;text-align:center}"
                ".info{background:#e8f5e9;padding:10px;border-radius:5px;margin:10px 0}"
                ".warning{background:#ffebee;padding:10px;border-radius:5px;margin:10px 0;color:#c62828}"
                "form{margin:20px 0}"
                "input[type='file']{width:100%;padding:10px;margin:10px 0;border:1px solid #ddd;border-radius:5px}"
                ".btn{background:#4caf50;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;width:100%}"
                ".btn:hover{background:#45a049}"
                "</style>"
                "</head><body>"
                "<div class='container'>"
                "<h1>🔄 Обновление прошивки</h1>"
                "<div class='info'>"
                "Размер флеш: " + String(flashSize) + " байт<br>"
                "Версия: " + String(firmwareVersion) + "</div>";
  if(Update.hasError()) {
    html += "<div class='warning'>Ошибка: " + String(Update.getError()) + "</div>";
  }
  html += "<form method='POST' action='/update' enctype='multipart/form-data'>"
          "<h2>ESP32 Прошивка</h2>"
          "<input type='file' name='update' accept='.bin'>"
          "<button class='btn'>Загрузить прошивку ESP</button>"
          "</form>";
  html += "<form method='POST' action='/updateSTM' enctype='multipart/form-data'>"
          "<h2>STM32 Прошивка</h2>"
          "<input type='file' name='stm32' accept='.bin'>"
          "<button class='btn' style='background:#2196F3'>Загрузить прошивку STM32</button>"
          "</form>"
          "</div></body></html>";
  httpServer.send(200, "text/html", html);
}

void handleUpdate() {
  HTTPUpload& upload = httpServer.upload();
  if(upload.status == UPLOAD_FILE_START) {
    Serial.println("Начало загрузки прошивки");
    if(!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Serial.println("Ошибка инициализации обновления");
      Update.printError(Serial);
      return;
    }
  }
  else if(upload.status == UPLOAD_FILE_WRITE) {
    Serial.printf("Загрузка: %u байт\n", upload.currentSize);
    if(Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
      return;
    }
  }
  else if(upload.status == UPLOAD_FILE_END) {
    if(Update.end(true)) {
      Serial.printf("Обновление завершено: %u байт\n", upload.totalSize);
      bot.sendMessage(CHAT_ID, "Прошивка успешно загружена. Перезагрузка...", "");
    }
  }
}

void handleUpdateSTM() {
  HTTPUpload& upload = httpServer.upload();
  static File stmFile;
  if (upload.status == UPLOAD_FILE_START) {
    firmwareUpdateSTMMode = true;
    if (stmFile) stmFile.close();
    stmFile = LittleFS.open("/firmware.bin", "w");
    if (!stmFile) {
      Serial.println("Ошибка открытия файла /firmware.bin для записи");
      bot.sendMessage(CHAT_ID, "Ошибка открытия файла для прошивки STM32", "");
      firmwareUpdateSTMMode = false;
      return;
    }
    Serial.println("Начало загрузки прошивки STM32");
    STM32_setPins(bootPin, resetPin);
    STM32_setSerial(&mySerial);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (stmFile) stmFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (stmFile) stmFile.close();
    Serial.println("Загрузка прошивки STM32 завершена, начинаем прошивку...");
    STM32_enterBootloader();
    bool flashOk = STM32_flashFirmware("/firmware.bin");
    if (flashOk) {
      bot.sendMessage(CHAT_ID, "✅ Прошивка STM32 завершена успешно", "");
      if (LittleFS.exists("/firmware.bin")) {
        LittleFS.remove("/firmware.bin");
        Serial.println("Удалён файл /firmware.bin после прошивки");
      }
      // Задержка перед ребутом
      delay(2000);
      
      // Ребут ESP32
      Serial.println("Ребут ESP32 после прошивки STM32");
      ESP.restart();
    } else {
      bot.sendMessage(CHAT_ID, "❌ Ошибка прошивки STM32", "");
      // Задержка перед ребутом
      delay(2000);
      
      // Ребут ESP32
      Serial.println("Ребут ESP32 после прошивки STM32");
      ESP.restart();
    }
    firmwareUpdateSTMMode = false;
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (stmFile) stmFile.close();
    if (LittleFS.exists("/firmware.bin")) {
      LittleFS.remove("/firmware.bin");
      Serial.println("Удалён файл /firmware.bin после прерванной загрузки");
    }
    firmwareUpdateSTMMode = false;
    Serial.println("Загрузка прошивки STM32 прервана");
    bot.sendMessage(CHAT_ID, "Загрузка прошивки STM32 прервана", "");
  }
}

void enterFirmwareUpdateMode() {
  firmwareUpdateMode = true;
  bot.sendMessage(CHAT_ID, "ESP в режиме прошивки. Перейдите по адресу http://" + WiFi.localIP().toString() + "/ для загрузки прошивки", "");
}

void exitFirmwareUpdateMode() {
  firmwareUpdateMode = false;
  bot.sendMessage(CHAT_ID, "ESP вышел из режима прошивки", "");
}

void setupWebServer() {
  httpServer.on("/", HTTP_GET, handleRoot);
  httpServer.on("/update", HTTP_POST, []() {
    httpServer.sendHeader("Connection", "close");
    httpServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    delay(1000);
    ESP.restart();
  }, handleUpdate);
  httpServer.on("/updateSTM", HTTP_POST, []() {
    httpServer.sendHeader("Connection", "close");
    httpServer.send(200, "text/plain", "STM32 update initiated");
  }, handleUpdateSTM);
  httpServer.begin();
  Serial.println("HTTP сервер запущен");
  Serial.println("Для загрузки прошивки перейдите по адресу: http://" + WiFi.localIP().toString());
}