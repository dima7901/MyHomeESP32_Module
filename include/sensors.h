#pragma once
#include <Arduino.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <RTClib.h>

extern Adafruit_BME280 bme;
extern RTC_DS3231 rtc;
extern bool rtcFound;

// Добавляем объявления глобальных переменных для датчиков
extern float bmeT, bmeH, bmeP, bmeT1, bmeH1, bmeP1;

void updateBME280Data();
