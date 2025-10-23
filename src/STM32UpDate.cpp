#include "STM32UpDate.h"
#include <LittleFS.h>

static uint8_t _boot0_pin = 37;
static uint8_t _reset_pin = 39;
static Stream* _stm32_serial = nullptr;

#define WRITE_ADDR 0x08000000
#define SIZE_WRITE 256
#define FIRMWARE_PATH "/firmware.bin"

#define STM32_ACK  0x79
#define STM32_NACK 0x1F

// ================== Вспомогательные =====================
static void clear_uart_buffer() {
  while (_stm32_serial && _stm32_serial->available()) {
    _stm32_serial->read();
  }
}

static uint8_t ack_byte(uint32_t timeout = 5000) {
  uint32_t start = millis();
  while (millis() - start < timeout) {
    if (_stm32_serial && _stm32_serial->available()) {
      uint8_t res = _stm32_serial->read();
      if (res == STM32_ACK) return 0;
      if (res == STM32_NACK) return 1;
    }
    delay(1);
  }
  return 1; // timeout
}

static uint8_t send_cmd(uint8_t cmd) {
  uint8_t arr[2] = { cmd, (uint8_t)(cmd ^ 0xFF) };
  _stm32_serial->write(arr, 2);
  _stm32_serial->flush();
  delay(5);
  return ack_byte();
}

static uint8_t send_address(uint32_t addr) {
  uint8_t buf[5];
  buf[0] = addr >> 24;
  buf[1] = (addr >> 16) & 0xFF;
  buf[2] = (addr >> 8) & 0xFF;
  buf[3] = addr & 0xFF;
  buf[4] = buf[0] ^ buf[1] ^ buf[2] ^ buf[3];
  _stm32_serial->write(buf, 5);
  _stm32_serial->flush();
  delay(5);
  return ack_byte();
}

// ================== BOOTLOADER =====================
void STM32_setPins(uint8_t boot0, uint8_t reset) {
  _boot0_pin = boot0;
  _reset_pin = reset;
  pinMode(_boot0_pin, OUTPUT);
  pinMode(_reset_pin, OUTPUT);
  digitalWrite(_boot0_pin, LOW);
  digitalWrite(_reset_pin, HIGH);
}

void STM32_setSerial(Stream* serial) {
  _stm32_serial = serial;
}

void STM32_enterBootloader() {
  Serial.println("=== Вход в Bootloader ===");
  digitalWrite(_boot0_pin, HIGH);
  delay(100);
  digitalWrite(_reset_pin, LOW);
  delay(100);
  digitalWrite(_reset_pin, HIGH);
  delay(300);

  ((HardwareSerial*)_stm32_serial)->updateBaudRate(57600);
  ((HardwareSerial*)_stm32_serial)->begin(57600, SERIAL_8E1);
  delay(200);

  clear_uart_buffer();
  _stm32_serial->write(0x7F);
  delay(200);

  if (ack_byte(1000) == 0)
    Serial.println("✅ Bootloader активен");
  else
    Serial.println("❌ Нет ответа от Bootloader");
}

void STM32_exitBootloader() {
  Serial.println("=== Выход из Bootloader ===");
  digitalWrite(_boot0_pin, LOW);
  delay(50);
  digitalWrite(_reset_pin, LOW);
  delay(50);
  digitalWrite(_reset_pin, HIGH);
  delay(100);
  ((HardwareSerial*)_stm32_serial)->updateBaudRate(115200);
  ((HardwareSerial*)_stm32_serial)->begin(115200, SERIAL_8E1);
  Serial.println("✅ Нормальный режим STM32");
}

// ================== Чтение ID =====================
bool STM32_getChipID() {
  Serial.println("🔍 Получение ID...");
  clear_uart_buffer();
  if (send_cmd(0x02) != 0) {
    Serial.println("⚠️ GetID не принят (пропускаем)");
    return false;
  }

  uint32_t t0 = millis();
  while (!_stm32_serial->available() && (millis() - t0 < 1000)) delay(1);
  if (!_stm32_serial->available()) {
    Serial.println("⚠️ Нет ответа на длину ID");
    return false;
  }

  uint8_t len = _stm32_serial->read();
  uint8_t id[3] = {0};
  for (int i = 0; i <= len; i++) {
    t0 = millis();
    while (!_stm32_serial->available() && (millis() - t0 < 1000)) delay(1);
    if (_stm32_serial->available()) id[i] = _stm32_serial->read();
  }

  ack_byte();
  Serial.printf("✅ Chip ID: 0x%02X%02X\n", id[0], id[1]);
  return true;
}

// ================== Стирание =====================
uint8_t STM32_eraseMemory() {
  Serial.println("🧹 Стирание памяти...");

  clear_uart_buffer();
  uint8_t res = send_cmd(0x43);  // обычное стирание
  if (res == 0) {
    Serial.println("📡 Команда 0x43 принята");
    uint8_t cmd[2] = {0xFF, 0x00};
    _stm32_serial->write(cmd, 2);
    _stm32_serial->flush();

    if (ack_byte(8000) == 0) { // STM32 стирает долго
      Serial.println("✅ Память стерта (0x43)");
      return 0;
    } else {
      Serial.println("❌ Ошибка ACK после 0x43, пробуем 0x44");
    }
  }

  // fallback — расширенное стирание
  clear_uart_buffer();
  res = send_cmd(0x44);
  if (res == 0) {
    Serial.println("📡 Команда 0x44 принята");
    uint8_t ext[3] = {0xFF, 0xFF, 0x00};
    _stm32_serial->write(ext, 3);
    _stm32_serial->flush();

    if (ack_byte(12000) == 0) {
      Serial.println("✅ Память стерта (0x44)");
      return 0;
    } else {
      Serial.println("❌ Ошибка ACK после 0x44");
    }
  }

  Serial.println("❌ Не удалось стереть память STM32");
  return 1;
}

// ================== Прошивка =====================
bool STM32_flashFirmware(const char* path) {
  if (!_stm32_serial) {
    Serial.println("❌ Serial STM32 не установлен");
    return false;
  }

  if (!LittleFS.exists(path)) {
    Serial.println("❌ Файл прошивки не найден");
    return false;
  }

  File f = LittleFS.open(path, "r");
  if (!f) {
    Serial.println("❌ Ошибка открытия файла");
    return false;
  }

  size_t total = f.size();
  if (total == 0) {
    Serial.println("❌ Пустой файл");
    f.close();
    return false;
  }

  STM32_getChipID();
  if (STM32_eraseMemory() != 0) {
    Serial.println("❌ Ошибка стирания памяти");
    f.close();
    return false;
  }

  Serial.printf("📝 Запись прошивки (%d байт)...\n", total);
  uint32_t addr = WRITE_ADDR;
  uint8_t cmd[2] = {0x31, 0xCE};

  while (f.available()) {
    clear_uart_buffer();

    if (send_cmd(0x31) != 0) {
      Serial.println("❌ Ошибка отправки команды записи");
      break;
    }

    if (send_address(addr) != 0) {
      Serial.println("❌ Ошибка отправки адреса");
      break;
    }

    uint8_t data[SIZE_WRITE];
    uint16_t len = f.read(data, SIZE_WRITE);
    uint16_t aligned = (len + 3) & ~3;
    uint8_t cs = aligned - 1;
    uint8_t buf[SIZE_WRITE + 2];

    buf[0] = aligned - 1;
    for (uint16_t i = 0; i < aligned; i++) {
      uint8_t b = (i < len) ? data[i] : 0xFF;
      buf[i + 1] = b;
      cs ^= b;
    }
    buf[aligned + 1] = cs;

    _stm32_serial->write(buf, aligned + 2);
    _stm32_serial->flush();

    if (ack_byte(1500) != 0) {
      Serial.println("❌ Ошибка ACK при записи блока");
      break;
    }

    addr += SIZE_WRITE;
    Serial.printf("📊 %d / %d байт (%.1f%%)\n", f.position(), total, (float)f.position() * 100 / total);
    delay(10);
  }

  f.close();
  STM32_exitBootloader();
  Serial.println("🎉 Прошивка завершена");
  return true;
}
