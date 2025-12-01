#include "storage.h"

Preferences prefs;

extern Statistics g_stats;
extern PourMode g_pourMode;
extern uint16_t g_targetVolume;
extern uint8_t g_selectedShot;

void loadSettings() {
    if (!prefs.begin(PREFS_NAMESPACE, false)) {
        Serial.println("ERROR: Failed to init preferences!");
        // Не критична помилка - продовжити з дефолтними значеннями
        return;
    }
    
    // Завантажити налаштування
    g_pourMode = (PourMode)prefs.getUChar("pourMode", MODE_MANUAL);
    g_targetVolume = prefs.getUShort("volume", VOLUME_DEFAULT);
    g_selectedShot = prefs.getUChar("shot", 1);
    
    // Завантажити статистику
    g_stats.totalPours = prefs.getUInt("totalPours", 0);
    g_stats.totalVolume = prefs.getUInt("totalVolume", 0);
    g_stats.totalTime = prefs.getUInt("totalTime", 0);
    g_stats.errors = prefs.getUInt("errors", 0);
    
    Serial.println("Settings loaded:");
    Serial.printf("  Mode: %s\n", g_pourMode == MODE_MANUAL ? "Manual" : "Auto");
    Serial.printf("  Volume: %d ml\n", g_targetVolume);
    Serial.printf("  Shot: %d\n", g_selectedShot);
    Serial.printf("  Total pours: %d\n", g_stats.totalPours);
    
    prefs.end();
}

void saveSettings() {
    if (!prefs.begin(PREFS_NAMESPACE, false)) {
        Serial.println("ERROR: Failed to save settings!");
        return;
    }
    
    prefs.putUChar("pourMode", g_pourMode);
    prefs.putUShort("volume", g_targetVolume);
    prefs.putUChar("shot", g_selectedShot);
    
    prefs.end();
    
    DEBUG_PRINTLN("Settings saved");
}

void saveStatistics() {
    if (!prefs.begin(PREFS_NAMESPACE, false)) {
        Serial.println("ERROR: Failed to save statistics!");
        return;
    }
    
    prefs.putUInt("totalPours", g_stats.totalPours);
    prefs.putUInt("totalVolume", g_stats.totalVolume);
    prefs.putUInt("totalTime", g_stats.totalTime);
    prefs.putUInt("errors", g_stats.errors);
    
    prefs.end();
    
    DEBUG_PRINTLN("Statistics saved");
}

void resetSettings() {
    if (!prefs.begin(PREFS_NAMESPACE, false)) {
        Serial.println("ERROR: Failed to reset settings!");
        return;
    }
    
    prefs.clear();
    prefs.end();
    
    // Встановити дефолтні значення
    g_pourMode = MODE_MANUAL;
    g_targetVolume = VOLUME_DEFAULT;
    g_selectedShot = 1;
    
    Serial.println("Settings reset to defaults");
}

void resetStatistics() {
    g_stats.totalPours = 0;
    g_stats.totalVolume = 0;
    g_stats.totalTime = 0;
    g_stats.errors = 0;
    g_stats.lastPourVolume = 0;
    g_stats.lastPourTime = 0;
    
    saveStatistics();
    
    Serial.println("Statistics reset");
}