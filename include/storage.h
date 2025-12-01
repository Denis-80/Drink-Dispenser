#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

// Завантаження/збереження налаштувань
void loadSettings();
void saveSettings();

// Статистика
void saveStatistics();
void resetStatistics();
void resetSettings();

#endif // STORAGE_H