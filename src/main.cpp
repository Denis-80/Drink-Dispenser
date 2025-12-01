#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "control.h"
#include "storage.h"

#if ENABLE_WIFI
#include "network.h"
#endif

// FreeRTOS Task Handles
TaskHandle_t uiTaskHandle = NULL;
TaskHandle_t controlTaskHandle = NULL;

// Глобальні змінні стану
SystemState g_systemState = STATE_IDLE;
PourMode g_pourMode = MODE_MANUAL;
uint16_t g_targetVolume = VOLUME_DEFAULT;
uint8_t g_selectedShot = 1;
bool g_glassPresent[5] = {false};
Statistics g_stats = {0};

// Час останнього збереження статистики
unsigned long lastStatsSave = 0;

// Forward declarations
void uiTask(void *parameter);
void controlTask(void *parameter);

void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n\n=== GyverDrink T4 Start ===");
    Serial.print("Firmware: v");
    Serial.println(FIRMWARE_VERSION);
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    
    // Ініціалізація дисплея
    Serial.print("Init display... ");
    if (!initDisplay()) {
        Serial.println("FAILED!");
        SAFE_RESTART();
    }
    Serial.println("OK");
    
    // Показати заставку
    showSplash();
    delay(2000);
    
    // Ініціалізація периферії
    Serial.print("Init peripherals... ");
    initPeripherals();
    Serial.println("OK");
    
    // Завантаження налаштувань
    Serial.print("Loading settings... ");
    loadSettings();
    Serial.println("OK");
    
#ifdef FACTORY_RESET
    Serial.println("FACTORY RESET!");
    resetSettings();
    delay(1000);
#endif
    
#if ENABLE_WIFI
    // Ініціалізація мережі
    Serial.print("Init network... ");
    setupNetwork();
    Serial.println("OK");
#endif
    
    // Створення задач FreeRTOS
    Serial.println("Creating tasks...");
    
    // UI задача (дисплей) на ядрі 0
    xTaskCreatePinnedToCore(
        uiTask,
        "UI_Task",
        STACK_SIZE_UI,
        NULL,
        PRIORITY_UI,
        &uiTaskHandle,
        CORE_UI
    );
    
    if (uiTaskHandle == NULL) {
        Serial.println("ERROR: Failed to create UI task!");
        SAFE_RESTART();
    }
    Serial.println("UI Task started on core 0");
    
    // Control задача (управління) на ядрі 1
    xTaskCreatePinnedToCore(
        controlTask,
        "Control_Task",
        STACK_SIZE_CONTROL,
        NULL,
        PRIORITY_CONTROL,
        &controlTaskHandle,
        CORE_CONTROL
    );
    
    if (controlTaskHandle == NULL) {
        Serial.println("ERROR: Failed to create Control task!");
        SAFE_RESTART();
    }
    Serial.println("Control Task started on core 1");
    
    Serial.println("Setup complete!");
    Serial.println("===================\n");
}

void loop() {
    // Головний цикл порожній - вся робота в задачах FreeRTOS
    
#if ENABLE_WIFI
    updateNetwork();
#endif
    
    // Періодичне збереження статистики
    if (millis() - lastStatsSave > STATS_SAVE_INTERVAL) {
        saveStatistics();
        lastStatsSave = millis();
    }
    
    // Debug info
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 10000) {
        Serial.printf("Heap: %d, State: %d, Volume: %d\n", 
            ESP.getFreeHeap(), g_systemState, g_targetVolume);
        lastDebug = millis();
    }
    
    delay(100);
}

// ========================================
// UI TASK - Оновлення дисплея
// ========================================
void uiTask(void *parameter) {
    Serial.println("UI Task running");
    
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(50); // 20 FPS
    
    while (true) {
        // Оновити дисплей
        updateDisplay(g_systemState, g_pourMode, g_targetVolume, g_selectedShot, g_glassPresent);
        
        // Перевірка стану пам'яті
        if (ESP.getFreeHeap() < 50000) {
            Serial.println("WARNING: Low memory!");
        }
        
        // Чекати до наступного оновлення
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

// ========================================
// CONTROL TASK - Управління розливом
// ========================================
void controlTask(void *parameter) {
    Serial.println("Control Task running");
    
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(10); // 100 Hz
    
    while (true) {
        // Обробка енкодера та кнопок
        updateControls();
        
        // Оновлення стану розливу
        updatePourState();
        
        // Оновлення LED
        updateLED(g_systemState, g_pourMode);
        
        // Чекати до наступного оновлення
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

// ========================================
// ДОПОМІЖНІ ФУНКЦІЇ
// ========================================

void stopAllTasks() {
    Serial.println("Stopping all tasks...");
    
    if (uiTaskHandle != NULL) {
        vTaskDelete(uiTaskHandle);
        uiTaskHandle = NULL;
    }
    
    if (controlTaskHandle != NULL) {
        vTaskDelete(controlTaskHandle);
        controlTaskHandle = NULL;
    }
    
    // Зупинити розлив
    stopPour();
}

// Serial команди для debug
void serialEvent() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "help") {
            Serial.println("\n=== Commands ===");
            Serial.println("help - List commands");
            Serial.println("stats - Show statistics");
            Serial.println("heap - Show memory");
            Serial.println("tasks - Show task info");
            Serial.println("reset - Reset statistics");
            Serial.println("restart - Restart device");
            Serial.println("pour X - Pour X ml");
            Serial.println("================\n");
        }
        else if (cmd == "stats") {
            Serial.println("\n=== Statistics ===");
            Serial.printf("Total pours: %d\n", g_stats.totalPours);
            Serial.printf("Total volume: %d ml\n", g_stats.totalVolume);
            Serial.printf("Total time: %d sec\n", g_stats.totalTime);
            Serial.printf("Errors: %d\n", g_stats.errors);
            Serial.println("==================\n");
        }
        else if (cmd == "heap") {
            Serial.println("\n=== Memory ===");
            Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
            Serial.printf("Min Free Heap: %d bytes\n", ESP.getMinFreeHeap());
            Serial.printf("Heap Size: %d bytes\n", ESP.getHeapSize());
            Serial.println("==============\n");
        }
        else if (cmd == "tasks") {
            Serial.println("\n=== FreeRTOS Tasks ===");
            Serial.printf("UI Task: %s\n", uiTaskHandle != NULL ? "Running" : "Stopped");
            Serial.printf("Control Task: %s\n", controlTaskHandle != NULL ? "Running" : "Stopped");
            Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
            Serial.println("======================\n");
        }
        else if (cmd == "reset") {
            resetStatistics();
            Serial.println("Statistics reset!");
        }
        else if (cmd == "restart") {
            Serial.println("Restarting...");
            delay(1000);
            esp_restart();
        }
        else if (cmd.startsWith("pour ")) {
            int vol = cmd.substring(5).toInt();
            if (vol >= VOLUME_MIN && vol <= VOLUME_MAX) {
                g_targetVolume = vol;
                startPour();
                Serial.printf("Pouring %d ml\n", vol);
            } else {
                Serial.println("Invalid volume!");
            }
        }
        else {
            Serial.println("Unknown command. Type 'help'");
        }
    }
}

