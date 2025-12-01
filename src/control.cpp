#include "control.h"

// Об'єкти
Servo servo;
CRGB leds[LED_COUNT];

// Стани енкодера
volatile int encoderPos = 0;
volatile bool encoderChanged = false;
int lastEncoderPos = 0;

// Стани кнопок
bool buttonStartPressed = false;
bool encoderButtonPressed = false;
unsigned long lastButtonPress = 0;
unsigned long lastEncoderPress = 0;

// Стан розливу
unsigned long pourStartTime = 0;
bool isPourActive = false;

// Зовнішні глобальні змінні
extern SystemState g_systemState;
extern PourMode g_pourMode;
extern uint16_t g_targetVolume;
extern uint8_t g_selectedShot;
extern bool g_glassPresent[5];
extern Statistics g_stats;

// Interrupt handlers
void IRAM_ATTR encoderISR() {
    static unsigned long lastInterrupt = 0;
    unsigned long now = millis();
    
    if (now - lastInterrupt < 5) return; // Debounce
    
    int clk = digitalRead(ENCODER_CLK);
    int dt = digitalRead(ENCODER_DT);
    
    if (clk != dt) {
        encoderPos++;
    } else {
        encoderPos--;
    }
    
    encoderChanged = true;
    lastInterrupt = now;
}

void initPeripherals() {
    // Енкодер
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), encoderISR, CHANGE);
    
    // Кнопка старт
    pinMode(BUTTON_START, INPUT);
    
    // Датчики рюмок
    pinMode(GLASS_PIN_1, INPUT);
    pinMode(GLASS_PIN_2, INPUT);
    pinMode(GLASS_PIN_3, INPUT);
    pinMode(GLASS_PIN_4, INPUT);
    pinMode(GLASS_PIN_5, INPUT);
    
    // Помпа
    pinMode(PUMP_POWER, OUTPUT);
    digitalWrite(PUMP_POWER, LOW);
    
    ledcSetup(PUMP_CHANNEL, PUMP_FREQ, 8);
    ledcAttachPin(PUMP_POWER, PUMP_CHANNEL);
    ledcWrite(PUMP_CHANNEL, 0);
    
    // Сервопривод
    pinMode(SERVO_POWER, OUTPUT);
    digitalWrite(SERVO_POWER, HIGH);
    
    servo.setPeriodHertz(50);
    servo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);
    servo.write(POS_PARKING);
    
    // LED стрічка
    FastLED.addLeds<LED_TYPE, LED_PIN, LED_ORDER>(leds, LED_COUNT);
    FastLED.setBrightness(LED_BRIGHTNESS);
    
    // Стартова анімація
    for (int i = 0; i < LED_COUNT; i++) {
        leds[i] = CHSV(LED_COLOR, 255, 255);
        FastLED.show();
        delay(50);
    }
}

void updateControls() {
    // Обробка енкодера
    if (encoderChanged) {
        int delta = encoderPos - lastEncoderPos;
        
        if (delta != 0) {
            // Зміна об'єму
            int newVolume = g_targetVolume + (delta * VOLUME_STEP);
            
            if (newVolume >= VOLUME_MIN && newVolume <= VOLUME_MAX) {
                g_targetVolume = newVolume;
                
                extern void saveSettings();
                saveSettings();
                
#if ENABLE_WIFI
                extern void broadcastState();
                broadcastState();
#endif
            }
            
            lastEncoderPos = encoderPos;
        }
        
        encoderChanged = false;
    }
    
    // Кнопка енкодера (вибір рюмки)
    if (digitalRead(ENCODER_SW) == LOW) {
        if (!encoderButtonPressed && (millis() - lastEncoderPress > DEBOUNCE_MS)) {
            encoderButtonPressed = true;
            lastEncoderPress = millis();
            
            // Наступна рюмка
            g_selectedShot++;
            if (g_selectedShot > 5) g_selectedShot = 1;
            
            DEBUG_PRINTF("Shot selected: %d\n", g_selectedShot);
            
            extern void saveSettings();
            saveSettings();
        }
    } else {
        encoderButtonPressed = false;
    }
    
    // Кнопка старт/стоп
    bool startButton = digitalRead(BUTTON_START);
    if (startButton == HIGH) {
        if (!buttonStartPressed && (millis() - lastButtonPress > DEBOUNCE_MS)) {
            buttonStartPressed = true;
            lastButtonPress = millis();
            
            if (g_systemState == STATE_IDLE || g_systemState == STATE_READY) {
                startPour();
            } else if (g_systemState == STATE_POURING) {
                stopPour();
            }
        }
    } else {
        buttonStartPressed = false;
    }
    
    // Оновити датчики рюмок
    g_glassPresent[0] = digitalRead(GLASS_PIN_1) == HIGH;
    g_glassPresent[1] = digitalRead(GLASS_PIN_2) == HIGH;
    g_glassPresent[2] = digitalRead(GLASS_PIN_3) == HIGH;
    g_glassPresent[3] = digitalRead(GLASS_PIN_4) == HIGH;
    g_glassPresent[4] = digitalRead(GLASS_PIN_5) == HIGH;
}

void updatePourState() {
    if (!isPourActive) return;
    
    unsigned long elapsed = millis() - pourStartTime;
    unsigned long pourTime = (g_targetVolume / PUMP_ML_PER_SEC) * 1000;
    
    // Перевірка таймауту
    if (elapsed > MAX_POUR_TIME) {
        Serial.println("ERROR: Pour timeout!");
        stopPour();
        g_systemState = STATE_ERROR;
        g_stats.errors++;
        return;
    }
    
    // Завершення розливу
    if (elapsed >= pourTime) {
        completePour();
    }
}

void startPour() {
    if (g_systemState == STATE_POURING) {
        Serial.println("Already pouring!");
        return;
    }
    
    // Перевірка рюмки
    if (!g_glassPresent[g_selectedShot - 1]) {
        Serial.println("No glass detected!");
        return;
    }
    
    Serial.printf("Starting pour: %d ml to shot %d\n", g_targetVolume, g_selectedShot);
    
    g_systemState = STATE_MOVING;
    
    // Рух до рюмки
    int position = 0;
    switch (g_selectedShot) {
        case 1: position = POS_SHOT_1; break;
        case 2: position = POS_SHOT_2; break;
        case 3: position = POS_SHOT_3; break;
        case 4: position = POS_SHOT_4; break;
        case 5: position = POS_SHOT_5; break;
    }
    
    servo.write(position);
    delay(500); // Чекати завершення руху
    
    // Почати розлив
    g_systemState = STATE_POURING;
    isPourActive = true;
    pourStartTime = millis();
    
    ledcWrite(PUMP_CHANNEL, PUMP_SPEED_DEFAULT);
    
#if ENABLE_WIFI
    extern void broadcastState();
    broadcastState();
#endif
}

void stopPour() {
    Serial.println("Stopping pour");
    
    // Зупинити помпу
    ledcWrite(PUMP_CHANNEL, 0);
    
    isPourActive = false;
    g_systemState = STATE_IDLE;
    
    // Повернення в паркінг
    servo.write(POS_PARKING);
    
#if ENABLE_WIFI
    extern void broadcastState();
    broadcastState();
#endif
}

void completePour() {
    Serial.println("Pour complete!");
    
    // Зупинити помпу
    ledcWrite(PUMP_CHANNEL, 0);
    
    // Оновити статистику
    g_stats.totalPours++;
    g_stats.totalVolume += g_targetVolume;
    g_stats.lastPourVolume = g_targetVolume;
    g_stats.lastPourTime = millis();
    
    isPourActive = false;
    
    // Повернення в паркінг
    servo.write(POS_PARKING);
    delay(500);
    
    g_systemState = STATE_IDLE;
    
    // В авто-режимі перейти до наступної рюмки
    if (g_pourMode == MODE_AUTO) {
        g_selectedShot++;
        if (g_selectedShot > 5) g_selectedShot = 1;
    }
    
    extern void saveStatistics();
    saveStatistics();
    
#if ENABLE_WIFI
    extern void broadcastState();
    broadcastState();
#endif
}

void updateLED(SystemState state, PourMode mode) {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < 50) return; // 20 FPS
    
    uint8_t hue = mode == MODE_MANUAL ? 160 : 96; // Синій / Зелений
    
    switch (state) {
        case STATE_IDLE:
        case STATE_READY:
            // Статичний колір
            fill_solid(leds, LED_COUNT, CHSV(hue, 255, 150));
            break;
            
        case STATE_MOVING:
            // Пульсація
            {
                uint8_t brightness = beatsin8(60, 100, 255);
                fill_solid(leds, LED_COUNT, CHSV(hue, 255, brightness));
            }
            break;
            
        case STATE_POURING:
            // Біжуча хвиля
            {
                uint8_t pos = beat8(60);
                for (int i = 0; i < LED_COUNT; i++) {
                    leds[i] = CHSV(hue, 255, cubicwave8(pos + i * 25));
                }
            }
            break;
            
        case STATE_ERROR:
            // Червоне мигання
            {
                uint8_t brightness = beat8(120) > 127 ? 255 : 0;
                fill_solid(leds, LED_COUNT, CRGB(brightness, 0, 0));
            }
            break;
            
        default:
            fill_solid(leds, LED_COUNT, CRGB::Black);
            break;
    }
    
    FastLED.show();
    lastUpdate = millis();
}

// API функції
void setTargetVolume(uint16_t vol) {
    if (vol >= VOLUME_MIN && vol <= VOLUME_MAX) {
        g_targetVolume = vol;
        
        extern void saveSettings();
        saveSettings();
        
        DEBUG_PRINTF("Volume set: %d ml\n", vol);
    }
}

void setPourMode(PourMode mode) {
    g_pourMode = mode;
    
    extern void saveSettings();
    saveSettings();
    
    DEBUG_PRINTF("Mode set: %s\n", mode == MODE_MANUAL ? "Manual" : "Auto");
}

void selectShot(uint8_t shot) {
    if (shot >= 1 && shot <= 5) {
        g_selectedShot = shot;
        
        extern void saveSettings();
        saveSettings();
        
        DEBUG_PRINTF("Shot selected: %d\n", shot);
    }
}