#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <Update.h>
#include <LittleFS.h>

extern WebServer httpServer;

void setupWebServer();
void enterFirmwareUpdateMode();
void exitFirmwareUpdateMode();
