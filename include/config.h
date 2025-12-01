#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ========================================
// 🎯 ОСНОВНІ НАЛАШТУВАННЯ
// ========================================

// Версія прошивки
#define FIRMWARE_VERSION "8.0.0"
#define DEVICE_NAME "GyverDrink"

// ========================================
// 🖥️ ДИСПЛЕЙ (LILYGO T4 - ST7789)
// ========================================
// Піни налаштовані в platformio.ini через TFT_eSPI

#define SCREEN_WIDTH 135
#define SCREEN_HEIGHT 240
#define SCREEN_ROTATION 1 // 0, 1, 2, 3

// Кольори (RGB565)
#define COLOR_BG        0x0000  // Чорний
#define COLOR_PRIMARY   0x07FF  // Cyan
#define COLOR_SUCCESS   0x07E0  // Зелений
#define COLOR_WARNING   0xFD20  // Помаранчевий
#define COLOR_ERROR     0xF800  // Червоний
#define COLOR_TEXT      0xFFFF  // Білий
#define COLOR_GRAY      0x7BEF  // Сірий

// ========================================
// 🕹️ УПРАВЛІННЯ
// ========================================

// Енкодер (вбудований на T4)
#define ENCODER_CLK   13   // GPIO13
#define ENCODER_DT    15   // GPIO15  
#define ENCODER_SW    0    // GPIO0 (Boot button)

// Кнопка старт/стоп
#define BUTTON_START  37   // GPIO37 (Input only)

// Debounce
#define DEBOUNCE_MS   50

// ========================================
// 🔌 ПЕРИФЕРІЯ
// ========================================

// Помпа
#define PUMP_POWER    33   // GPIO33 - живлення помпи
#define PUMP_FREQ     5000 // PWM частота
#define PUMP_CHANNEL  0    // PWM канал

// Сервопривід
#define SERVO_POWER   25   // GPIO25 - живлення серво
#define SERVO_PIN     26   // GPIO26 - сигнал серво
#define SERVO_MIN_US  500
#define SERVO_MAX_US  2500

// Датчики рюмок (Input only pins)
#define GLASS_PIN_1   35   // GPIO35
#define GLASS_PIN_2   36   // GPIO36
#define GLASS_PIN_3   37   // GPIO37
#define GLASS_PIN_4   38   // GPIO38
#define GLASS_PIN_5   39   // GPIO39

// LED стрічка WS2812B
#define LED_PIN       12   // GPIO12
#define LED_COUNT     10   // Кількість світлодіодів
#define LED_TYPE      WS2812B
#define LED_ORDER     GRB
#define LED_BRIGHTNESS 100 // 0-255
#define LED_COLOR     200  // Hue 0-255

// ========================================
// ⚙️ НАЛАШТУВАННЯ РОЗЛИВУ
// ========================================

// Об'єми (мл)
#define VOLUME_MIN    10
#define VOLUME_MAX    200
#define VOLUME_STEP   5
#define VOLUME_DEFAULT 25

// Калібрування помпи
#define PUMP_ML_PER_SEC 10.0  // мл/сек (потрібно калібрувати!)
#define PUMP_SPEED_DEFAULT 255 // PWM 0-255

// Позиції сервопривода (градуси)
#define POS_SHOT_1    30
#define POS_SHOT_2    60
#define POS_SHOT_3    90
#define POS_SHOT_4    120
#define POS_SHOT_5    150
#define POS_PARKING   0    // Паркувальна позиція

// Режими роботи
enum PourMode {
    MODE_MANUAL = 0,  // Ручний - вибір рюмки
    MODE_AUTO = 1     // Авто - по черзі
};

// ========================================
// 📡 WI-FI
// ========================================

#define ENABLE_WIFI   1  // 1 = увімкнути, 0 = вимкнути

// Access Point (за замовчуванням)
#define AP_SSID       "Nalivator-Setup"
#define AP_PASS       "12345678"
#define AP_CHANNEL    6
#define AP_HIDDEN     false
#define AP_MAX_CONN   4

// Station Mode (підключення до роутера)
#define WIFI_SSID     "YourWiFiName"
#define WIFI_PASS     "YourPassword"
#define WIFI_TIMEOUT  10000  // ms

// IP адреси
#define AP_IP         IPAddress(192, 168, 4, 1)
#define AP_GATEWAY    IPAddress(192, 168, 4, 1)
#define AP_SUBNET     IPAddress(255, 255, 255, 0)

// ========================================
// 🌐 WEB SERVER
// ========================================

#define WEB_PORT      80
#define WS_PORT       81  // WebSocket

// ========================================
// 🔄 OTA UPDATE
// ========================================

#define ENABLE_OTA    1
#define OTA_PASSWORD  "admin"
#define OTA_PORT      3232

// ========================================
// 💾 ЗБЕРІГАННЯ
// ========================================

#define PREFS_NAMESPACE "gyverdrink"
#define STATS_SAVE_INTERVAL 30000  // Зберігати статистику кожні 30 сек

// ========================================
// 🧵 MULTITASKING (FreeRTOS)
// ========================================

// Розміри стеків (байти)
#define STACK_SIZE_UI       16384  // UI задача
#define STACK_SIZE_CONTROL  8192   // Control задача
#define STACK_SIZE_NETWORK  8192   // Network задача

// Пріоритети (0-24, більше = вищий)
#define PRIORITY_UI         1      // Нижчий
#define PRIORITY_CONTROL    2      // Вищий
#define PRIORITY_NETWORK    1      // Нижчий

// Ядра CPU (0 або 1)
#define CORE_UI             0      // UI на ядрі 0
#define CORE_CONTROL        1      // Control на ядрі 1
#define CORE_NETWORK        0      // Network на ядрі 0

// ========================================
// 🐛 DEBUG
// ========================================

#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED 0
#endif

#if DEBUG_ENABLED
  #define DEBUG_PRINT(x)   Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(...)  Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

// ========================================
// ⚠️ БЕЗПЕКА
// ========================================

// Обмеження
#define MAX_POUR_TIME     30000    // Максимальний час розливу (мс)
#define WATCHDOG_TIMEOUT  10000    // Watchdog таймаут (мс)

// Перезавантаження при критичній помилці
#define SAFE_RESTART() do { \
    Serial.println("CRITICAL ERROR - RESTARTING..."); \
    delay(1000); \
    esp_restart(); \
} while(0)

// Перевірка з автоперезавантаженням
#define CHECK_OR_RESTART(condition, msg) do { \
    if (!(condition)) { \
        Serial.println("ERROR: " msg); \
        SAFE_RESTART(); \
    } \
} while(0)

// ========================================
// 🏭 FACTORY RESET
// ========================================

// Розкоментуйте для скидання налаштувань при наступній прошивці
// #define FACTORY_RESET

// ========================================
// 📊 СТАТИСТИКА
// ========================================

struct Statistics {
    uint32_t totalPours;        // Загальна кількість розливів
    uint32_t totalVolume;       // Загальний об'єм (мл)
    uint32_t totalTime;         // Загальний час роботи (сек)
    uint32_t errors;            // Кількість помилок
    uint32_t lastPourVolume;    // Останній об'єм
    uint32_t lastPourTime;      // Час останнього розливу
};

// ========================================
// 🎮 СТАНИ СИСТЕМИ
// ========================================

enum SystemState {
    STATE_IDLE = 0,       // Очікування
    STATE_READY,          // Готовий до розливу
    STATE_MOVING,         // Рух серво
    STATE_POURING,        // Розлив
    STATE_PAUSED,         // Пауза
    STATE_ERROR,          // Помилка
    STATE_CLEANING        // Очищення
};

// ========================================
// 🔔 ПОДІЇ
// ========================================

enum EventType {
    EVENT_NONE = 0,
    EVENT_VOLUME_CHANGED,
    EVENT_MODE_CHANGED,
    EVENT_SHOT_SELECTED,
    EVENT_POUR_START,
    EVENT_POUR_STOP,
    EVENT_POUR_COMPLETE,
    EVENT_ERROR,
    EVENT_SETTINGS_CHANGED
};

#endif // CONFIG_H