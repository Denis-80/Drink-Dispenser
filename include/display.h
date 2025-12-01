#ifndef DISPLAY_H
#define DISPLAY_H

#include <TFT_eSPI.h>
#include "config.h"

// Ініціалізація дисплея
bool initDisplay();

// Показати заставку
void showSplash();

// Оновити дисплей
void updateDisplay(SystemState state, PourMode mode, uint16_t volume, uint8_t shot, bool glasses[5]);

// Малювання компонентів
void drawStatusBar(SystemState state, PourMode mode);
void drawVolume(uint16_t volume);
void drawShotSelector(uint8_t selected, bool glasses[5]);
void drawProgress(uint8_t percent);

// Показати помилку
void showError(const char* message);

#endif // DISPLAY_H