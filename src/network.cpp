#include "network.h"

#if ENABLE_WIFI

AsyncWebServer server(WEB_PORT);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

extern SystemState g_systemState;
extern PourMode g_pourMode;
extern uint16_t g_targetVolume;
extern uint8_t g_selectedShot;
extern bool g_glassPresent[5];
extern Statistics g_stats;

// HTML сторінка
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>GyverDrink Control</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #fff;
            padding: 20px;
        }
        .container {
            max-width: 600px;
            margin: 0 auto;
            background: rgba(255,255,255,0.1);
            backdrop-filter: blur(10px);
            border-radius: 20px;
            padding: 30px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.3);
        }
        h1 {
            text-align: center;
            margin-bottom: 30px;
            font-size: 2.5em;
        }
        .status {
            background: rgba(255,255,255,0.2);
            padding: 20px;
            border-radius: 15px;
            margin-bottom: 20px;
        }
        .control-group {
            margin: 20px 0;
        }
        label {
            display: block;
            margin-bottom: 10px;
            font-weight: bold;
        }
        input[type="range"] {
            width: 100%;
            height: 8px;
            border-radius: 5px;
            background: rgba(255,255,255,0.3);
            outline: none;
        }
        .volume-display {
            text-align: center;
            font-size: 3em;
            font-weight: bold;
            margin: 20px 0;
        }
        .btn {
            width: 100%;
            padding: 15px;
            font-size: 1.2em;
            border: none;
            border-radius: 10px;
            cursor: pointer;
            margin: 10px 0;
            transition: all 0.3s;
        }
        .btn-primary {
            background: #4CAF50;
            color: white;
        }
        .btn-danger {
            background: #f44336;
            color: white;
        }
        .btn:hover {
            transform: scale(1.05);
        }
        .btn:active {
            transform: scale(0.95);
        }
        .shot-selector {
            display: grid;
            grid-template-columns: repeat(5, 1fr);
            gap: 10px;
            margin: 20px 0;
        }
        .shot-btn {
            padding: 20px 10px;
            background: rgba(255,255,255,0.2);
            border: 2px solid transparent;
            border-radius: 10px;
            cursor: pointer;
            transition: all 0.3s;
        }
        .shot-btn.active {
            border-color: #4CAF50;
            background: rgba(76,175,80,0.3);
        }
        .shot-btn.has-glass {
            border-color: #2196F3;
        }
        .stats {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 15px;
            margin-top: 20px;
        }
        .stat-item {
            background: rgba(255,255,255,0.2);
            padding: 15px;
            border-radius: 10px;
            text-align: center;
        }
        .stat-value {
            font-size: 2em;
            font-weight: bold;
        }
        .stat-label {
            font-size: 0.9em;
            opacity: 0.8;
        }
        .mode-toggle {
            display: flex;
            gap: 10px;
            margin: 20px 0;
        }
        .mode-toggle button {
            flex: 1;
            padding: 15px;
            background: rgba(255,255,255,0.2);
            border: 2px solid transparent;
            border-radius: 10px;
            color: white;
            cursor: pointer;
            transition: all 0.3s;
        }
        .mode-toggle button.active {
            border-color: #4CAF50;
            background: rgba(76,175,80,0.3);
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🍺 GyverDrink</h1>
        
        <div class="status">
            <div>Статус: <span id="status">Очікування</span></div>
            <div>Режим: <span id="mode">Ручний</span></div>
        </div>

        <div class="mode-toggle">
            <button id="btnManual" class="active" onclick="setMode(0)">Ручний</button>
            <button id="btnAuto" onclick="setMode(1)">Авто</button>
        </div>

        <div class="control-group">
            <label>Об'єм (мл):</label>
            <div class="volume-display" id="volumeDisplay">25</div>
            <input type="range" id="volumeSlider" min="10" max="200" step="5" value="25" oninput="updateVolume(this.value)">
        </div>

        <div class="control-group">
            <label>Вибір рюмки:</label>
            <div class="shot-selector">
                <div class="shot-btn" id="shot1" onclick="selectShot(1)">1</div>
                <div class="shot-btn" id="shot2" onclick="selectShot(2)">2</div>
                <div class="shot-btn" id="shot3" onclick="selectShot(3)">3</div>
                <div class="shot-btn" id="shot4" onclick="selectShot(4)">4</div>
                <div class="shot-btn" id="shot5" onclick="selectShot(5)">5</div>
            </div>
        </div>

        <button class="btn btn-primary" onclick="startPour()">▶️ Налити</button>
        <button class="btn btn-danger" onclick="stopPour()">⏹️ Стоп</button>

        <div class="stats">
            <div class="stat-item">
                <div class="stat-value" id="totalPours">0</div>
                <div class="stat-label">Розливів</div>
            </div>
            <div class="stat-item">
                <div class="stat-value" id="totalVolume">0</div>
                <div class="stat-label">мл всього</div>
            </div>
            <div class="stat-item">
                <div class="stat-value" id="uptime">0:00</div>
                <div class="stat-label">Час роботи</div>
            </div>
            <div class="stat-item">
                <div class="stat-value" id="freeHeap">0</div>
                <div class="stat-label">RAM вільно</div>
            </div>
        </div>
    </div>

    <script>
        var gateway = `ws://${window.location.hostname}/ws`;
        var websocket;

        function initWebSocket() {
            console.log('Connecting to WebSocket...');
            websocket = new WebSocket(gateway);
            websocket.onopen = onOpen;
            websocket.onclose = onClose;
            websocket.onmessage = onMessage;
        }

        function onOpen(event) {
            console.log('Connection opened');
        }

        function onClose(event) {
            console.log('Connection closed');
            setTimeout(initWebSocket, 2000);
        }

        function onMessage(event) {
            var data = JSON.parse(event.data);
            console.log('Received:', data);
            
            if (data.status !== undefined) {
                document.getElementById('status').textContent = data.status;
            }
            if (data.mode !== undefined) {
                document.getElementById('mode').textContent = data.mode == 0 ? 'Ручний' : 'Авто';
                document.getElementById('btnManual').classList.toggle('active', data.mode == 0);
                document.getElementById('btnAuto').classList.toggle('active', data.mode == 1);
            }
            if (data.volume !== undefined) {
                document.getElementById('volumeDisplay').textContent = data.volume;
                document.getElementById('volumeSlider').value = data.volume;
            }
            if (data.shot !== undefined) {
                for (let i = 1; i <= 5; i++) {
                    document.getElementById('shot' + i).classList.toggle('active', i == data.shot);
                }
            }
            if (data.glasses !== undefined) {
                for (let i = 0; i < 5; i++) {
                    document.getElementById('shot' + (i+1)).classList.toggle('has-glass', data.glasses[i]);
                }
            }
            if (data.stats !== undefined) {
                document.getElementById('totalPours').textContent = data.stats.pours;
                document.getElementById('totalVolume').textContent = data.stats.volume;
            }
            if (data.uptime !== undefined) {
                let hours = Math.floor(data.uptime / 3600);
                let minutes = Math.floor((data.uptime % 3600) / 60);
                document.getElementById('uptime').textContent = hours + ':' + (minutes < 10 ? '0' : '') + minutes;
            }
            if (data.heap !== undefined) {
                document.getElementById('freeHeap').textContent = Math.floor(data.heap / 1024) + 'K';
            }
        }

        function updateVolume(value) {
            document.getElementById('volumeDisplay').textContent = value;
            websocket.send(JSON.stringify({cmd: 'volume', value: parseInt(value)}));
        }

        function setMode(mode) {
            websocket.send(JSON.stringify({cmd: 'mode', value: mode}));
        }

        function selectShot(shot) {
            websocket.send(JSON.stringify({cmd: 'shot', value: shot}));
        }

        function startPour() {
            websocket.send(JSON.stringify({cmd: 'start'}));
        }

        function stopPour() {
            websocket.send(JSON.stringify({cmd: 'stop'}));
        }

        window.addEventListener('load', onLoad);
        function onLoad(event) {
            initWebSocket();
        }
    </script>
</body>
</html>
)rawliteral";

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        
        // Відправити поточний стан
        DynamicJsonDocument doc(512);
        doc["status"] = getStateString(g_systemState);
        doc["mode"] = g_pourMode;
        doc["volume"] = g_targetVolume;
        doc["shot"] = g_selectedShot;
        
        JsonArray glasses = doc.createNestedArray("glasses");
        for (int i = 0; i < 5; i++) {
            glasses.add(g_glassPresent[i]);
        }
        
        JsonObject stats = doc.createNestedObject("stats");
        stats["pours"] = g_stats.totalPours;
        stats["volume"] = g_stats.totalVolume;
        
        doc["uptime"] = millis() / 1000;
        doc["heap"] = ESP.getFreeHeap();
        
        String response;
        serializeJson(doc, response);
        client->text(response);
        
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            data[len] = 0;
            
            DynamicJsonDocument doc(256);
            DeserializationError error = deserializeJson(doc, (char*)data);
            
            if (!error) {
                String cmd = doc["cmd"];
                
                if (cmd == "volume") {
                    extern void setTargetVolume(uint16_t vol);
                    setTargetVolume(doc["value"]);
                } 
                else if (cmd == "mode") {
                    extern void setPourMode(PourMode mode);
                    setPourMode((PourMode)(int)doc["value"]);
                } 
                else if (cmd == "shot") {
                    extern void selectShot(uint8_t shot);
                    selectShot(doc["value"]);
                } 
                else if (cmd == "start") {
                    extern void startPour();
                    startPour();
                } 
                else if (cmd == "stop") {
                    extern void stopPour();
                    stopPour();
                }
            }
        }
    }
}

void setupNetwork() {
    Serial.println("Setting up network...");
    
    WiFi.mode(WIFI_AP);
    
    // Налаштування IP адреси
    if (!WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET)) {
        Serial.println("AP Config Failed!");
        return;
    }
    
    // Створення точки доступу
    if (!WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONN)) {
        Serial.println("AP Start Failed!");
        return;
    }
    
    Serial.println("Access Point started!");
    Serial.print("SSID: ");
    Serial.println(AP_SSID);
    Serial.print("Password: ");
    Serial.println(AP_PASS);
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
    
    // WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    
    // Events
    server.addHandler(&events);
    
    // Головна сторінка
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });
    
    // API endpoints
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
        DynamicJsonDocument doc(512);
        doc["state"] = getStateString(g_systemState);
        doc["mode"] = g_pourMode;
        doc["volume"] = g_targetVolume;
        doc["shot"] = g_selectedShot;
        doc["uptime"] = millis() / 1000;
        doc["heap"] = ESP.getFreeHeap();
        
        JsonObject stats = doc.createNestedObject("stats");
        stats["totalPours"] = g_stats.totalPours;
        stats["totalVolume"] = g_stats.totalVolume;
        stats["errors"] = g_stats.errors;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.on("/api/start", HTTP_POST, [](AsyncWebServerRequest *request){
        extern void startPour();
        startPour();
        request->send(200, "text/plain", "OK");
    });
    
    server.on("/api/stop", HTTP_POST, [](AsyncWebServerRequest *request){
        extern void stopPour();
        stopPour();
        request->send(200, "text/plain", "OK");
    });
    
    // 404
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not found");
    });
    
    // Запуск сервера
    server.begin();
    Serial.println("HTTP server started");
    
#if ENABLE_OTA
    setupOTA();
#endif
}

void updateNetwork() {
    ws.cleanupClients();
    
#if ENABLE_OTA
    ArduinoOTA.handle();
#endif
}

void broadcastState() {
    DynamicJsonDocument doc(512);
    doc["status"] = getStateString(g_systemState);
    doc["mode"] = g_pourMode;
    doc["volume"] = g_targetVolume;
    doc["shot"] = g_selectedShot;
    
    JsonArray glasses = doc.createNestedArray("glasses");
    for (int i = 0; i < 5; i++) {
        glasses.add(g_glassPresent[i]);
    }
    
    JsonObject stats = doc.createNestedObject("stats");
    stats["pours"] = g_stats.totalPours;
    stats["volume"] = g_stats.totalVolume;
    
    doc["uptime"] = millis() / 1000;
    doc["heap"] = ESP.getFreeHeap();
    
    String response;
    serializeJson(doc, response);
    ws.textAll(response);
}

const char* getStateString(SystemState state) {
    switch (state) {
        case STATE_IDLE: return "Очікування";
        case STATE_READY: return "Готовий";
        case STATE_MOVING: return "Рух";
        case STATE_POURING: return "Розлив";
        case STATE_PAUSED: return "Пауза";
        case STATE_ERROR: return "Помилка";
        case STATE_CLEANING: return "Очищення";
        default: return "Невідомо";
    }
}

#if ENABLE_OTA
void setupOTA() {
    ArduinoOTA.setHostname(DEVICE_NAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.setPort(OTA_PORT);
    
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("Start updating " + type);
        
        // Зупинити всі задачі
        extern void stopAllTasks();
        stopAllTasks();
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    
    ArduinoOTA.begin();
    Serial.println("OTA ready");
}
#endif

#endif // ENABLE_WIFI