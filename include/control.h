#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include <FastLED.h>
#include "config.h"

// Ініціалізація периферії
void initPeripherals();

// Оновлення управління
void updateControls();
void updatePourState();

// Управління розливом
void startPour();
void stopPour();
void completePour();

// LED ефекти
void updateLED(SystemState state, PourMode mode);

// API функції
void setTargetVolume(uint16_t vol);
void setPourMode(PourMode mode);
void selectShot(uint8_t shot);

#endif // CONTROL_H