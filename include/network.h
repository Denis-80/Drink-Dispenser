#ifndef NETWORK_H
#define NETWORK_H

#include "config.h"

#if ENABLE_WIFI

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

#if ENABLE_OTA
#include <ArduinoOTA.h>
#endif

// Ініціалізація мережі
void setupNetwork();

// Оновлення мережі
void updateNetwork();

// Broadcast стану
void broadcastState();

// Допоміжні функції
const char* getStateString(SystemState state);

#if ENABLE_OTA
void setupOTA();
#endif

#endif // ENABLE_WIFI

#endif // NETWORK_H