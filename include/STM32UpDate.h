#pragma once
#include <Arduino.h>
#include <FS.h>

void STM32_setPins(uint8_t boot0, uint8_t reset);
void STM32_setSerial(Stream* serial);
void STM32_enterBootloader();
void STM32_exitBootloader();
bool STM32_getChipID();
uint8_t STM32_eraseMemory();
bool STM32_flashFirmware(const char* path);