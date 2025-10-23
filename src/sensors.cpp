#include "sensors.h"

//==============Объявление датчиков=======================//
Adafruit_BME280 bme;
RTC_DS3231 rtc;
bool rtcFound = false;

void updateBME280Data() {
  bmeT = bme.readTemperature();
  bmeH = bme.readHumidity();
  bmeP = bme.readPressure() / 133.322;
  if (isnan(bmeT) || isnan(bmeH) || isnan(bmeP)) {
    Serial.println("Ошибка чтения данных с BME280");
  }
}
