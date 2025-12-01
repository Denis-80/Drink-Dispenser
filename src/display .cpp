#include "display.h"

TFT_eSPI tft = TFT_eSPI();

bool initDisplay() {
    Serial.println("[DISPLAY] Starting init...");
    
    Serial.println("[DISPLAY] Calling tft.init()...");
    tft.init();
    Serial.println("[DISPLAY] tft.init() - OK");
    
    Serial.printf("[DISPLAY] Setting rotation to %d\n", SCREEN_ROTATION);
    tft.setRotation(SCREEN_ROTATION);
    Serial.println("[DISPLAY] Rotation set - OK");
    
    Serial.println("[DISPLAY] Filling screen with BG color...");
    tft.fillScreen(COLOR_BG);
    Serial.println("[DISPLAY] Screen fill - OK");
    
    // Увімкнути підсвітку
    Serial.println("[DISPLAY] Enabling backlight (pin 4)...");
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    Serial.println("[DISPLAY] Backlight - OK");
    
    Serial.println("[DISPLAY] Drawing test rectangle...");
    tft.drawRect(10, 10, 100, 100, TFT_RED);
    Serial.println("[DISPLAY] Test rectangle - OK");
    
    Serial.println("[DISPLAY] Drawing test text...");
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(20, 50);
    tft.print("TEST");
    Serial.println("[DISPLAY] Test text - OK");
    
    Serial.println("[DISPLAY] Init complete!");
    return true;
}

void showSplash() {
    Serial.println("[DISPLAY] Showing splash screen...");
    
    // Тестовые цвета
    tft.fillScreen(TFT_BLACK);
    delay(500);
    tft.fillScreen(TFT_RED);
    delay(500);
    tft.fillScreen(TFT_GREEN);
    delay(500);
    tft.fillScreen(TFT_BLUE);
    delay(500);
    tft.fillScreen(TFT_WHITE);
    delay(500);
    
    tft.fillScreen(COLOR_BG);
    
    // Заголовок
    tft.setTextColor(COLOR_PRIMARY);
    tft.setTextSize(2);
    tft.setCursor(20, 80);
    tft.println("GyverDrink");
    
    // Версія
    tft.setTextColor(COLOR_GRAY);
    tft.setTextSize(1);
    tft.setCursor(35, 110);
    tft.print("v");
    tft.println(FIRMWARE_VERSION);
    
    // Анімація
    for (int i = 0; i < 3; i++) {
        tft.fillCircle(SCREEN_WIDTH/2 - 20 + i*20, 140, 4, COLOR_SUCCESS);
        delay(300);
    }
    
    Serial.println("[DISPLAY] Splash screen complete");
}

void updateDisplay(SystemState state, PourMode mode, uint16_t volume, uint8_t shot, bool glasses[5]) {
    static SystemState lastState = STATE_IDLE;
    static PourMode lastMode = MODE_MANUAL;
    static uint16_t lastVolume = 0;
    static uint8_t lastShot = 0;
    
    // Перемальовувати тільки при зміні
    bool needRedraw = (state != lastState || mode != lastMode || 
                       volume != lastVolume || shot != lastShot);
    
    if (needRedraw) {
        tft.fillScreen(COLOR_BG);
        
        // Верхня панель - статус
        drawStatusBar(state, mode);
        
        // Об'єм
        drawVolume(volume);
        
        // Вибір рюмки
        drawShotSelector(shot, glasses);
        
        // Прогрес (якщо розлив)
        if (state == STATE_POURING) {
            drawProgress(50); // TODO: реальний прогрес
        }
        
        lastState = state;
        lastMode = mode;
        lastVolume = volume;
        lastShot = shot;
    }
}

void drawStatusBar(SystemState state, PourMode mode) {
    // Фон статус-бару
    tft.fillRect(0, 0, SCREEN_WIDTH, 30, COLOR_PRIMARY);
    
    // Статус
    tft.setTextColor(COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(5, 10);
    
    switch (state) {
        case STATE_IDLE:
            tft.print("Idle");
            break;
        case STATE_READY:
            tft.print("Ready");
            break;
        case STATE_MOVING:
            tft.print("Moving");
            break;
        case STATE_POURING:
            tft.print("Pouring");
            break;
        case STATE_PAUSED:
            tft.print("Paused");
            break;
        case STATE_ERROR:
            tft.print("ERROR");
            break;
        case STATE_CLEANING:
            tft.print("Cleaning");
            break;
    }
    
    // Режим
    tft.setCursor(SCREEN_WIDTH - 50, 10);
    tft.print(mode == MODE_MANUAL ? "MANUAL" : "AUTO");
}

void drawVolume(uint16_t volume) {
    // Центральний дисплей об'єму
    tft.setTextColor(COLOR_PRIMARY);
    tft.setTextSize(4);
    
    // Центрувати текст
    String volStr = String(volume);
    int16_t x = (SCREEN_WIDTH - volStr.length() * 24) / 2;
    
    tft.setCursor(x, 80);
    tft.print(volume);
    
    // "ml"
    tft.setTextSize(2);
    tft.setCursor(x + volStr.length() * 24 + 5, 95);
    tft.print("ml");
}

void drawShotSelector(uint8_t selected, bool glasses[5]) {
    int startY = 140;
    int spacing = 25;
    int radius = 10;
    
    for (int i = 0; i < 5; i++) {
        int x = 15 + i * spacing;
        int y = startY;
        
        // Колір залежно від вибору та наявності рюмки
        uint16_t color;
        if (i + 1 == selected) {
            color = COLOR_SUCCESS;  // Вибрана
        } else if (glasses[i]) {
            color = COLOR_PRIMARY;  // Є рюмка
        } else {
            color = COLOR_GRAY;     // Порожня
        }
        
        // Малювати кружок
        tft.fillCircle(x, y, radius, color);
        
        // Номер
        tft.setTextColor(COLOR_BG);
        tft.setTextSize(1);
        tft.setCursor(x - 3, y - 4);
        tft.print(i + 1);
    }
}

void drawProgress(uint8_t percent) {
    int barY = 200;
    int barHeight = 15;
    int barWidth = SCREEN_WIDTH - 20;
    
    // Фон прогрес-бару
    tft.drawRect(10, barY, barWidth, barHeight, COLOR_PRIMARY);
    
    // Заповнення
    int fillWidth = (barWidth - 4) * percent / 100;
    tft.fillRect(12, barY + 2, fillWidth, barHeight - 4, COLOR_SUCCESS);
    
    // Відсоток
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(1);
    tft.setCursor(SCREEN_WIDTH/2 - 10, barY + 20);
    tft.print(percent);
    tft.print("%");
}

void showError(const char* message) {
    tft.fillScreen(COLOR_ERROR);
    
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(2);
    tft.setCursor(20, 100);
    tft.println("ERROR!");
    
    tft.setTextSize(1);
    tft.setCursor(10, 130);
    tft.println(message);
    
    delay(3000);
}