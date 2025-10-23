#pragma once
#include <Arduino.h>

void safeDelay(unsigned long ms);
void printMemoryInfo();
bool saveSettings();
bool loadSettings();
void rebootController();
void setupWiFi();
