/*
 * Autobud Grow Manager v3.0
 * Version: Beta 1.6
 * * CHANGES:
 * - REMOVED: WiFi.disconnect() from setup (caused connection failures on some routers).
 * - ADDED: WiFi.setAutoReconnect(true).
 * - CHANGED: Increased WiFi connection timeout to 30 seconds.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>

#define FW_VERSION "Beta 1.6"

// ==========================================
//               GLOBALS & CONFIG
// ==========================================

// Defaults
const char* AP_SSID = "Autobud_Setup";
const char* AP_PASS = "grow4life";

// Web Server
AsyncWebServer server(80);
DNSServer dnsServer;
WiFiClient wifiClient;

// Flags & State
bool localOnly = false;
bool shouldSaveConfig = false;
bool shouldSaveState = false;
bool pumpRunning = false;
bool shouldReboot = false; 
unsigned long pumpStartTime = 0;
unsigned long pumpDuration = 0;

// Timers
unsigned long lastHourMillis = 0;
const unsigned long HOUR_MS = 3600000; // 3600000 = 1 hour

// --- Main Configuration Object ---
struct Config {
  String wifiSSID;
  String wifiPass;
  String apiAddress;
  String alertAddress;
  String deviceID;
  
  // Generic / Martin Jerry Pinout Defaults
  int pumpPin = 12;     // Relay
  int ledPin = 5;       // LED (Blue usually)
  int buttonPin = 13;   // Main Button
  
  int waterIntervalHours = 24;
  float waterAmount = 1.0;     
  float waterCal = 4.0;        
  float tankVolume = 120.0;
  int dryThreshold = 20;
  int lightHours = 18;
};

// --- Main State Object ---
struct State {
  int growDay = 1;
  int currentHour = 1;
  int vegDays = 0;
  int flowerDays = 0;
  int waterNextHour = 0;
  float tankLevel = 120.0;
  bool triggerDryWater = false;
};

// Initialize Globals
Config config;
State growState;

// Mock Sensor Data
float sensorTemp = 72.5;
float sensorHum = 55.0;
float sensorMoist = 45.0;

// ==========================================
//               WEB INTERFACE
// ==========================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Autobud Manager</title>
  <style>
    body { font-family: -apple-system, sans-serif; background: #f4f4f4; color: #333; margin: 0; padding: 20px; line-height: 1.5; }
    .container { max-width: 800px; margin: auto; background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
    h1, h2 { color: #2e7d32; }
    .card { background: #fafafa; border: 1px solid #ddd; padding: 15px; margin-bottom: 20px; border-radius: 6px; }
    .row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }
    label { font-weight: bold; font-size: 0.9em; }
    input[type=text], input[type=number], input[type=password] { padding: 8px; border: 1px solid #ccc; border-radius: 4px; width: 100px; }
    input.long { width: 200px; }
    button { background: #2e7d32; color: white; border: none; padding: 10px 15px; border-radius: 4px; cursor: pointer; font-size: 1em; }
    button:hover { background: #1b5e20; }
    button.danger { background: #c62828; }
    .status-bar { background: #e8f5e9; padding: 10px; border-radius: 4px; margin-bottom: 20px; display: flex; justify-content: space-between; font-weight: bold; }
    .nav { margin-bottom: 20px; }
    .nav a { margin-right: 15px; color: #555; text-decoration: none; font-weight: bold; }
    .fw-tag { font-size: 0.8em; color: #777; background:#eee; padding:2px 5px; border-radius:4px; vertical-align: middle;}
  </style>
</head>
<body>
  <div class="container">
    <div class="nav">
        <span style="font-size:1.2em; font-weight:bold; color:#2e7d32;">Autobud <span id="disp_fw" class="fw-tag">Loading...</span></span>
        <span style="float:right">
            <a href="/">Home</a>
            <a href="/update">Update FW</a>
            <a href="#" onclick="doReboot()">Reboot Device</a>
        </span>
    </div>

    <div class="status-bar">
        <span>Day <span id="disp_day">--</span></span>
        <span>Hour <span id="disp_hour">--</span></span>
        <span>Next Water: <span id="disp_next">--</span>h</span>
    </div>

    <div class="card">
        <h2>📊 Live Status</h2>
        <div class="row"><span>Tank Level:</span> <strong id="disp_tank">--</strong></div>
        <div class="row"><span>Temp / Hum:</span> <strong><span id="disp_temp">--</span>°F / <span id="disp_hum">--</span>%</strong></div>
        <div class="row"><span>Soil Moisture:</span> <strong id="disp_moist">--</strong></div>
        <div class="row">
            <button onclick="triggerAction('/api/pump')">Run Pump (Manual)</button>
            <button onclick="triggerAction('/api/refill')">Refill Tank Reset</button>
        </div>
    </div>

    <div class="card">
        <h2>💧 Irrigation Settings</h2>
        <div class="row">
            <label>Interval (Hours)</label>
            <input type="number" id="waterInterval" step="1">
        </div>
        <div class="row">
            <label>Amount (Oz)</label>
            <input type="number" id="waterAmount" step="0.1">
        </div>
        <div class="row">
            <label>Calibration (Sec/Oz)</label>
            <input type="number" id="waterCal" step="0.1">
        </div>
        <div class="row">
            <label>Tank Volume (Oz)</label>
            <input type="number" id="tankVolume" step="1">
        </div>
         <div class="row">
            <label>Dry Threshold (%)</label>
            <input type="number" id="dryThreshold" step="1">
        </div>
        <button onclick="saveSettings()">Save Irrigation Config</button>
    </div>

    <div class="card">
        <h2>💡 Light & System</h2>
        <div class="row">
            <label>Light On Duration (Hrs)</label>
            <input type="number" id="lightHours">
        </div>
        <div class="row">
            <label>Shift Day (Current Day)</label>
            <input type="number" id="growDay">
        </div>
         <div class="row">
            <label>Shift Hour (Current Hour)</label>
            <input type="number" id="currentHour">
        </div>
        <button onclick="saveSettings()">Save System Config</button>
    </div>

    <div class="card">
        <h2>⚙️ Hardware & Network</h2>
        <div class="row"><label>Pump GPIO</label><input type="number" id="pumpPin"></div>
        <div class="row"><label>LED GPIO</label><input type="number" id="ledPin"></div>
        <div class="row"><label>Button GPIO</label><input type="number" id="buttonPin"></div>
        <hr style="border:0; border-top:1px solid #ddd; margin:15px 0;">
        <div class="row"><label>SSID</label><input type="text" id="wifiSSID" class="long"></div>
        <div class="row"><label>Pass</label><input type="password" id="wifiPass" class="long" placeholder="Keep Empty to Unchanged"></div>
        <div class="row"><label>API URL</label><input type="text" id="apiAddress" class="long"></div>
        <div class="row"><label>Device ID</label><input type="text" id="deviceID" class="long"></div>
        <button onclick="saveSettings()">Save Hardware & Network</button>
        <div style="font-size:0.8em; color:#666; margin-top:5px;">Note: Network changes require a manual Reboot to take effect.</div>
    </div>
  </div>

<script>
    // Variables to track editing state
    let isEditing = false;
    const inputs = document.querySelectorAll('input');
    inputs.forEach(input => {
        input.addEventListener('focus', () => isEditing = true);
        input.addEventListener('blur', () => isEditing = false);
    });

    function fetchStatus() {
        if(isEditing) return; // Don't overwrite while typing

        fetch('/api/data', { cache: "no-store" })
            .then(r => r.json())
            .then(data => {
                document.getElementById('disp_fw').innerText = data.fw;
                
                // Update Displays
                document.getElementById('disp_day').innerText = data.state.growDay;
                document.getElementById('disp_hour').innerText = data.state.currentHour;
                document.getElementById('disp_next').innerText = data.state.waterNextHour;
                document.getElementById('disp_tank').innerText = data.state.tankLevel.toFixed(1) + " oz";
                document.getElementById('disp_temp').innerText = data.sensor.temp;
                document.getElementById('disp_hum').innerText = data.sensor.hum;
                document.getElementById('disp_moist').innerText = data.sensor.moist;

                // Update Inputs (Only if not focused, but we check isEditing above)
                if(document.activeElement.tagName !== "INPUT") {
                    document.getElementById('waterInterval').value = data.config.waterInterval;
                    document.getElementById('waterAmount').value = data.config.waterAmount;
                    document.getElementById('waterCal').value = data.config.waterCal;
                    document.getElementById('tankVolume').value = data.config.tankVolume;
                    document.getElementById('dryThreshold').value = data.config.dryThreshold;
                    document.getElementById('lightHours').value = data.config.lightHours;
                    
                    document.getElementById('pumpPin').value = data.config.pumpPin;
                    document.getElementById('ledPin').value = data.config.ledPin;
                    document.getElementById('buttonPin').value = data.config.buttonPin;

                    document.getElementById('wifiSSID').value = data.config.wifiSSID;
                    document.getElementById('apiAddress').value = data.config.apiAddress;
                    document.getElementById('deviceID').value = data.config.deviceID;
                    
                    document.getElementById('growDay').value = data.state.growDay;
                    document.getElementById('currentHour').value = data.state.currentHour;
                }
            })
            .catch(e => console.log("Data Fetch Error (ignoring if rebooting)", e));
    }

    // Helper: Get value or Default (Prevents NaN)
    const getInt = (id) => parseInt(document.getElementById(id).value) || 0;
    const getFloat = (id) => parseFloat(document.getElementById(id).value) || 0.0;
    const getStr = (id) => document.getElementById(id).value;

    function saveSettings() {
        const payload = {
            waterInterval: getInt('waterInterval'),
            waterAmount: getFloat('waterAmount'),
            waterCal: getFloat('waterCal'),
            tankVolume: getFloat('tankVolume'),
            dryThreshold: getInt('dryThreshold'),
            lightHours: getInt('lightHours'),
            
            pumpPin: getInt('pumpPin'),
            ledPin: getInt('ledPin'),
            buttonPin: getInt('buttonPin'),
            
            growDay: getInt('growDay'),
            currentHour: getInt('currentHour'),
            wifiSSID: getStr('wifiSSID'),
            wifiPass: getStr('wifiPass'),
            apiAddress: getStr('apiAddress'),
            deviceID: getStr('deviceID')
        };

        fetch('/api/save', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify(payload)
        }).then(r => { 
            alert('Settings Saved to Memory.'); 
            // Force a fetch immediately to verify
            setTimeout(fetchStatus, 500);
        });
    }

    function triggerAction(url) {
        fetch(url).then(r => alert('Command Sent'));
    }

    function doReboot() {
        if(confirm("Reboot Device now?")) {
            fetch('/api/reboot').then(() => {
                alert("Device Rebooting. Please reconnect to WiFi in 20s.");
            });
        }
    }

    // Initial Load
    fetchStatus();
    // Auto Refresh Loop (Every 3 seconds)
    setInterval(fetchStatus, 3000);

</script>
</body>
</html>
)rawliteral";

// ==========================================
//            FILESYSTEM FUNCTIONS
// ==========================================

void loadData() {
  if (LittleFS.exists("/config.json")) {
    File file = LittleFS.open("/config.json", "r");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, file);
    file.close();

    config.wifiSSID = doc["ssid"] | "";
    config.wifiPass = doc["pass"] | "";
    config.apiAddress = doc["api"] | "";
    config.deviceID = doc["devid"] | "AB-001";
    
    config.pumpPin = doc["pumpPin"] | 12;
    config.ledPin = doc["ledPin"] | 5;
    config.buttonPin = doc["buttonPin"] | 13;

    config.waterIntervalHours = doc["wInt"] | 24;
    config.waterAmount = doc["wAmt"] | 1.0;
    config.waterCal = doc["wCal"] | 4.0;
    config.tankVolume = doc["vol"] | 120.0;
    config.dryThreshold = doc["dryT"] | 20;
    config.lightHours = doc["lHrs"] | 18;
    
    Serial.print("FS: Loaded SSID: ["); Serial.print(config.wifiSSID); Serial.println("]");
    // Security: Don't print full pass, just length
    Serial.print("FS: Loaded Pass Length: "); Serial.println(config.wifiPass.length());
  } else {
    Serial.println("FS: No config.json found (Fresh Install)");
  }

  if (LittleFS.exists("/state.json")) {
    File file = LittleFS.open("/state.json", "r");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, file);
    file.close();

    growState.growDay = doc["day"] | 1;
    growState.currentHour = doc["hour"] | 1;
    growState.vegDays = doc["veg"] | 0;
    growState.flowerDays = doc["flower"] | 0;
    growState.waterNextHour = doc["wNext"] | 0;
    growState.tankLevel = doc["tank"] | config.tankVolume;
  }
}

void saveData() {
  // Save Config
  if (shouldSaveConfig) {
    DynamicJsonDocument doc(1024);
    doc["ssid"] = config.wifiSSID;
    doc["pass"] = config.wifiPass;
    doc["api"] = config.apiAddress;
    doc["devid"] = config.deviceID;
    
    doc["pumpPin"] = config.pumpPin;
    doc["ledPin"] = config.ledPin;
    doc["buttonPin"] = config.buttonPin;

    doc["wInt"] = config.waterIntervalHours;
    doc["wAmt"] = config.waterAmount;
    doc["wCal"] = config.waterCal;
    doc["vol"] = config.tankVolume;
    doc["dryT"] = config.dryThreshold;
    doc["lHrs"] = config.lightHours;

    File file = LittleFS.open("/config.json", "w");
    if(serializeJson(doc, file) == 0) {
      Serial.println("FS: Failed to write config.json");
    } else {
      Serial.println("FS: Config Saved Successfully");
    }
    file.close(); // Forces flush to storage
    shouldSaveConfig = false;
  }

  // Save State
  if (shouldSaveState) {
    DynamicJsonDocument doc(512);
    doc["day"] = growState.growDay;
    doc["hour"] = growState.currentHour;
    doc["veg"] = growState.vegDays;
    doc["flower"] = growState.flowerDays;
    doc["wNext"] = growState.waterNextHour;
    doc["tank"] = growState.tankLevel;

    File file = LittleFS.open("/state.json", "w");
    serializeJson(doc, file);
    file.close();
    shouldSaveState = false;
  }
}

// ==========================================
//               LOGIC & API
// ==========================================

void updateLighting() {
  bool lightsOn = (growState.currentHour < config.lightHours);
  // Active High or Low? Assuming Active High for now
  digitalWrite(config.ledPin, lightsOn ? HIGH : LOW);
}

void triggerPump(float amountOz) {
  if (pumpRunning) return; // Already running
  
  float durationSecs = amountOz * config.waterCal;
  pumpDuration = (unsigned long)(durationSecs * 1000);
  pumpStartTime = millis();
  
  digitalWrite(config.pumpPin, HIGH); // PUMP ON
  pumpRunning = true;
  
  // Deduct from tank
  growState.tankLevel -= amountOz;
  shouldSaveState = true;
  
  Serial.printf("Pump ON for %.1f sec (%.1f oz)\n", durationSecs, amountOz);
}

void handlePumpLogic() {
  if (pumpRunning) {
    if (millis() - pumpStartTime >= pumpDuration) {
      digitalWrite(config.pumpPin, LOW); // PUMP OFF
      pumpRunning = false;
      Serial.println("Pump OFF");
    }
  }
}

void postApiData() {
    if(localOnly || config.apiAddress == "") return;
    
    HTTPClient http;
    WiFiClient client;
    http.begin(client, config.apiAddress); // User requested just a JSON dump
    http.addHeader("Content-Type", "application/json");
    
    DynamicJsonDocument doc(1024);
    doc["id"] = config.deviceID;
    doc["day"] = growState.growDay;
    doc["hour"] = growState.currentHour;
    doc["tank"] = growState.tankLevel;
    doc["temp"] = sensorTemp;
    doc["hum"] = sensorHum;
    doc["fw"] = FW_VERSION;
    
    String json;
    serializeJson(doc, json);
    
    int rc = http.POST(json);
    Serial.printf("API Post: %d\n", rc);
    http.end();
}

void processHour() {
  growState.currentHour++;
  
  // Daily Rollover
  if (growState.currentHour > 24) {
    growState.currentHour = 1;
    growState.growDay++;
    if(config.lightHours <= 12) growState.flowerDays++;
    else growState.vegDays++;
    
    // Daily tasks could go here
    Serial.println("--- New Day ---");
  }
  
  // Check Water
  if (growState.waterNextHour <= 0) {
    triggerPump(config.waterAmount);
    growState.waterNextHour = config.waterIntervalHours;
  } else {
    growState.waterNextHour--;
  }

  // Update Lighting
  updateLighting();
  
  // Send Data
  postApiData();
  
  // Save State
  shouldSaveState = true;
}

// ==========================================
//               SETUP
// ==========================================

void setup() {
  Serial.begin(115200);
  delay(1000); 
  Serial.println("\n\n--- Autobud Booting ---");
  
  LittleFS.begin();
  loadData();

  // Hardware Pins Init
  Serial.printf("Pins: Pump=%d, LED=%d, Button=%d\n", config.pumpPin, config.ledPin, config.buttonPin);
  pinMode(config.pumpPin, OUTPUT);
  pinMode(config.ledPin, OUTPUT);
  digitalWrite(config.pumpPin, LOW);
  updateLighting();

  // ----------------------------------------------------
  // SAFETY CHECK: Force AP Mode if Button Held at Boot
  // ----------------------------------------------------
  pinMode(config.buttonPin, INPUT_PULLUP);
  int btnState = digitalRead(config.buttonPin);
  Serial.printf("Button Pin State: %d (0=PRESSED)\n", btnState);
  
  if(btnState == LOW) {
    delay(100);
    if(digitalRead(config.buttonPin) == LOW) {
        Serial.println("!!! Button Held - FORCING AP MODE !!!");
        localOnly = true;
    }
  }
  // ----------------------------------------------------

  // Network Setup (Skip if safety check triggered)
  if (!localOnly && config.wifiSSID != "") {
    Serial.print("Connecting to WiFi: ");
    Serial.println(config.wifiSSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.hostname("Autobud-Manager");
    // REMOVED: WiFi.disconnect(); // It was causing issues
    WiFi.setAutoReconnect(true); // Ensure it tries to reconnect
    delay(100);
    WiFi.begin(config.wifiSSID.c_str(), config.wifiPass.c_str());
    
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 60) { // Increased to 30s
      delay(500); Serial.print("."); retries++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi Connected: " + WiFi.localIP().toString());
    } else {
      Serial.print("\nWiFi Failed. Status: ");
      Serial.println(WiFi.status());
      localOnly = true;
    }
  } else {
    Serial.println("No SSID or Safety Trigger. AP Mode.");
    localOnly = true;
  }

  if (localOnly) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    dnsServer.start(53, "*", WiFi.softAPIP());
    Serial.println("AP Mode Started: IP 192.168.4.1");
  }

  // --- Web Routes ---
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // API: Get Data (JSON)
  server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(2048);
    
    doc["fw"] = FW_VERSION; // Send FW Version

    JsonObject c = doc.createNestedObject("config");
    c["waterInterval"] = config.waterIntervalHours;
    c["waterAmount"] = config.waterAmount;
    c["waterCal"] = config.waterCal;
    c["tankVolume"] = config.tankVolume;
    c["dryThreshold"] = config.dryThreshold;
    c["lightHours"] = config.lightHours;
    
    c["pumpPin"] = config.pumpPin;
    c["ledPin"] = config.ledPin;
    c["buttonPin"] = config.buttonPin;

    c["wifiSSID"] = config.wifiSSID;
    c["apiAddress"] = config.apiAddress;
    c["deviceID"] = config.deviceID;

    JsonObject s = doc.createNestedObject("state");
    s["growDay"] = growState.growDay;
    s["currentHour"] = growState.currentHour;
    s["waterNextHour"] = growState.waterNextHour;
    s["tankLevel"] = growState.tankLevel;
    
    JsonObject sn = doc.createNestedObject("sensor");
    sn["temp"] = sensorTemp;
    sn["hum"] = sensorHum;
    sn["moist"] = sensorMoist;

    String res;
    serializeJson(doc, res);
    request->send(200, "application/json", res);
  });

  // API: Save Config
  server.on("/api/save", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    Serial.println("API: Received Config Save Request");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, data);

    config.waterIntervalHours = doc["waterInterval"];
    config.waterAmount = doc["waterAmount"]; 
    config.waterCal = doc["waterCal"];      
    config.tankVolume = doc["tankVolume"];
    config.dryThreshold = doc["dryThreshold"];
    config.lightHours = doc["lightHours"];
    
    // Hardware
    int oldPump = config.pumpPin;
    int oldLed = config.ledPin;
    config.pumpPin = doc["pumpPin"];
    config.ledPin = doc["ledPin"];
    config.buttonPin = doc["buttonPin"];
    
    // If Pins changed, reset them immediately
    if(oldPump != config.pumpPin) {
        pinMode(oldPump, INPUT); pinMode(config.pumpPin, OUTPUT); digitalWrite(config.pumpPin, LOW);
    }
    if(oldLed != config.ledPin) {
        pinMode(oldLed, INPUT); pinMode(config.ledPin, OUTPUT);
    }

    // Input Sanitization for State
    int newDay = doc["growDay"];
    int newHour = doc["currentHour"];
    
    // Only update if valid (>0) to prevent reset bugs
    if(newDay > 0) growState.growDay = newDay;
    if(newHour > 0) growState.currentHour = newHour;
    
    String newSSID = doc["wifiSSID"];
    String newPass = doc["wifiPass"];
    
    if(newSSID.length() > 0) {
        Serial.print("API: Updating SSID to: "); Serial.println(newSSID);
        config.wifiSSID = newSSID;
        
        if(newPass.length() > 0) {
            Serial.println("API: Password Updated");
            config.wifiPass = newPass; 
        }
    }
    config.apiAddress = doc["apiAddress"] | "";
    config.deviceID = doc["deviceID"] | "";

    shouldSaveConfig = true;
    shouldSaveState = true;
    request->send(200, "text/plain", "Saved");
  });

  // API: Commands
  server.on("/api/pump", HTTP_GET, [](AsyncWebServerRequest *request){
    triggerPump(config.waterAmount); // Manual Trigger
    request->send(200, "text/plain", "OK");
  });
  
  server.on("/api/refill", HTTP_GET, [](AsyncWebServerRequest *request){
    growState.tankLevel = config.tankVolume;
    shouldSaveState = true;
    request->send(200, "text/plain", "OK");
  });

  server.on("/api/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
    shouldReboot = true; // Set Flag
    request->send(200, "text/plain", "Rebooting...");
  });

  // Captive Portal
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  AsyncElegantOTA.begin(&server);
  server.begin();
  lastHourMillis = millis();
}

// ==========================================
//               MAIN LOOP
// ==========================================

void loop() {
  AsyncElegantOTA.loop();
  if (localOnly) dnsServer.processNextRequest();

  // --- REBOOT HANDLER ---
  if (shouldReboot) {
    delay(2000); // Wait for web response to finish
    Serial.println("Rebooting via Web Command...");
    ESP.restart();
  }

  // Non-Blocking Pump
  handlePumpLogic();

  // Hour Timer
  if (millis() - lastHourMillis >= HOUR_MS) {
    lastHourMillis = millis();
    processHour();
  }

  // Data Persistence
  saveData();
}
