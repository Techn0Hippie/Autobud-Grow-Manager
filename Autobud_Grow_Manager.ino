#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "Arduino.h"
#include <AsyncElegantOTA.h>
#include <FS.h>
#include <ESPAsyncTCP.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h> 

// --- Hardware Pins ---
const int RESET_BUTTON_GPIO = 0; 

// --- Web Server & WiFi ---
AsyncWebServer server(80);
WiFiClient wifiClient; 
const char* AP_SSID = "Autobud";
const char* AP_PASS = "Grow4Life";
bool localonly = false; 
DNSServer dnsServer; 

// --- Global Settings (Loaded from SPIFFS) ---
String wifiSSID = "";
String wifiPassword = "";
String apiAddress = "";
String alertAddress = "";
String deviceID = "";
String macAddress = "";
int pump_gpio = 4;
int led_gpio = 5; 
int waterInterval = 24;
int waterAmount = 1;
float waterCal = 4.0;
int waterTankVolume = 120;
int dryThreshold = 20;
int lightHours = 18;

// --- Global State (Loaded from SPIFFS, updated by code) ---
int growDay = 1;
int hourCount = 1;
int vegDays = 0;
int flowerDays = 0;
int waterNextHour = 0;
float waterTankLevel = 0.0;

// --- Sensor Data (from external) ---
float temp = 0.0;
float hum = 0.0;
float moist = 0.0;

// --- Flags (Internal state, not saved) ---
bool waterRequestFlag = false;
bool dataRequestFlag = false;
bool triggerDryWater = false;

// --- Timers ---
unsigned long lastTime = 0;
unsigned long timerDelay = 3600000; // 1 hour
//unsigned long timerDelay = 10000; // 10-second test

// --- Web UI Parameter Names ---
const char* PARAM_INT = "inputInt";
const char* PARAM_WATERTM = "waterInt";
const char* PARAM_VOLUME = "volume";
const char* PARAM_THREASHOLD = "threashold";
const char* PARAM_LEDOFFSET = "ledoffset";
const char* PARAM_DAYADVANCE = "dayadvance";
const char* PARAM_DAY = "GrowDay";
const char* PARAM_WATER = "WaterRequest";
const char* PARAM_WATERNXT = "WaterNXT";
const char* PARAM_TEMP = "temp";
const char* PARAM_HUM = "hum";
const char* PARAM_MOIST = "moist";
const char* PARAM_ABADDESS = "abaddress";
const char* PARAM_WATERCAL = "watercal";
const char* PARAM_ABCAMADDESS = "abcamaddress";
const char* PARAM_APIADDRESS = "apiaddress";
const char* PARAM_ALERT = "alertadd";
const char* PARAM_LIGHT = "lightInt";
const char* PARAM_DEVID = "devid";
const char* PARAM_PUMPGPIO = "pumpgpio";
const char* PARAM_LEDGPIO = "ledgpio";
const char* PARAM_WIFI_SSID = "WiFiSSID";
const char* PARAM_WIFI_PASS = "WiFiPassword";

// Paths to API endpoint to post data
const char* serverName = "/upload";
const char* daycount = "/daycount";
const char* imgapi = "/imgupload";
const char* logapi = "/log";
const char* alertapi = "/emailalert";

// --- HTML Page ---
const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE HTML>
<style type"text/css">
html,body,div,span,object,iframe,h1,h2,h3,h4,h5,h6,p,blockquote,pre,abbr,address,cite,code,del,dfn,em,img,ins,kbd,q,samp,small,strong,sub,sup,var,b,i,dl,dt,dd,ol,ul,li,fieldset,form,label,legend,table,caption,tbody,tfoot,thead,tr,th,td,article,aside,canvas,details,figcaption,figure,footer,header,hgroup,menu,nav,section,summary,time,mark,audio,video{margin:0;padding:0;border:0;outline:0;font-size:100%;vertical-align:baseline;background:transparent}
body{line-height:1; font-family: 'Times New Roman', Times, serif;}
h1{font-size: 25px;}h2{font-size: 21px;}h3{font-size: 18px;}h4{font-size: 16px;}
article,aside,details,figcaption,figure,footer,header,hgroup,menu,nav,section{display:block}
nav ul{list-style:none}
blockquote,q{quotes:none}
blockquote:before,blockquote:after,q:before,q:after{content:none}
a{margin:0;padding:0;font-size:100%;vertical-align:baseline;background:transparent}
ins{background-color:#ff9;color:#000;text-decoration:none}
mark{background-color:#ff9;color:#000;font-style:italic;font-weight:bold}
del{text-decoration:line-through}
abbr[title],dfn[title]{border-bottom:1px dotted;cursor:help}
table{border-collapse:collapse;border-spacing:0}
hr{display:block;height:1px;border:0;border-top:1px solid #ccc;margin:1em 0;padding:0}
input,select{vertical-align:middle}
body,html { background-color: #FFF; }
header { background: #b1b6b8; padding: 20px 30px; max-width: 940px; margin: auto; }
nav { display: inline-block; }
nav ul li a { background: #EEE; padding: 2px 10px; font-size: 20px; text-decoration: none; font-weight: bold; color: #444; border-radius: 10px; }
nav ul li a:hover { background: #FFF; }
nav ul li { display: inline-block; margin: 10px; }
nav ul { list-style: none; }
#pageContent { max-width: 1000px; margin: auto; border: none; }
main { float: left; width: 60%; }
aside { float: right; width: 30%; }
article { border-bottom: 2px dotted #999; padding-bottom: 20px; margin-bottom: 20px; }
article h2 { font-weight: normal; margin-bottom: 12px; }
article p { }
main section { }
footer { background: #b1b6b8; max-width: 1000px; margin: auto; clear: both; text-align: right; }
footer p { padding: 20px; }
address { padding: 10px 20px 30px 10px; }
aside > div { margin: 10px auto; min-height: 100px; }
#logo { display: inline-block; vertical-align: middle; font-size: 30px; margin-right: 75px; color: #FFF; }
#logo img { width: 50px; vertical-align: middle; margin-right: 10px; }
body > section { max-width: 1000px; margin: auto; padding: 30px 0px; border-bottom: 1px solid #999; color: #333; }
@media screen and (max-width: 1000px) { body > section { padding: 30px 20px; } }
@media screen and (max-width: 600px) { main { float: none; width: 100%; } aside { float: none; width: 100%; } }
</style>
<html lang="en" class="no-js">
<meta charset="utf-8">
<meta http-equiv="x-ua-compatible" content="ie=edge">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Autobud</title>
</head>
<body>
  <header>
    <div id="logo"><img src="/logo.png">Autobud&nbsp;GrowManager</div>
    <nav>  
      <ul>
        <li><a href="/">Home</a></li>
        <li><a href="/update">Update Firmware</a></li>
        <li><a href="/post">Reboot</a></li>
        <li><a href="/start">Restart Grow</a></li>
      </ul>
    </nav>
  </header>
  <section>
    <strong>Beta Version: 0.971 (Refactored)</strong>
  </section>
  <section id="pageContent">
    <main role="main">
      <article>
        <h2>Auto Water</h2>
        <p>
        <form action="/get" target="_top">
        Auto Water: (Water Every %inputInt% Hours): <input type="number" name="inputInt">
        <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
        <form action="/get" target="hidden-form">
        Water Amount (oz): (Current: %waterInt%): <input type="number" name="waterInt">
        <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
        <form action="/get" target="hidden-form">
        Water Calibration (%watercal% seconds per oz): <input type="number" name="watercal">
        <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
        <form action="/get" target="hidden-form">
        Water Tank Volume ( %volume% oz): <input type="number" name="volume">
        <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
        <form action="/get" target="hidden-form">
        Dry Soil Threashold ( %threashold% Precent Dry): <input type="number" name="threashold">
        <input type="submit" value="Submit" onclick="submitMessage()">
        </p>
      </article>
      <article>
        <h2>Auto Lighting</h2>
        <p>
        <form action="/get" target="hidden-form">
          Light Hours: (Current: %lightInt%): <input type="number" name="lightInt">
          <input type="submit" value="Submit" onclick="submitMessage()">
          </form><br>
        </p>
      </article>
      <article>
        <h2>Auto Override</h2>
        <p>
          <button onclick="PumpControl();">Water</button>
          <button onclick="refill();">Refill Water Tank</button>
          <br><br>
        <form action="/get" target="hidden-form">
          Grow Hour Shift: <input type="number" name="ledoffset">
          <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
        <form action="/get" target="hidden-form">
          Grow Day Shift: <input type="number" name="dayadvance">
          <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
        </p>
      </article>
      <article>
        <h2>Connections & Configuration</h2>
        <p>
        <form action="/get" target="hidden-form">
          WiFi SSID (%WiFiSSID%): <input type="text" name="WiFiSSID">
          <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
        <form action="/get" target="hidden-form">
          WiFi Password (****): <input type="text" name="WiFiPassword">
          <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
         <form action="/get" target="hidden-form">
          API Endpoint: (Current: %apiaddress%): <input type="text" name="apiaddress">
          <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
         <form action="/get" target="hidden-form">
          Alert Text/Email: (Current: %alertadd%): <input type="text" name="alertadd">
          <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
        <form action="/get" target="hidden-form">
          Device ID: (Current: %devid%): <input type="text" name="devid">
          <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
        <form action="/get" target="hidden-form">
          Pump GPIO: (Current: %pumpgpio%): <input type="text" name="pumpgpio">
          <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
        <form action="/get" target="hidden-form">
          LED GPIO: (Current: %ledgpio%): <input type="text" name="ledgpio">
          <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
        </p>
      </article>
    </main>
    <aside>
      <div>Grow Status<br><br>
      DeviceID: %devid%<br><br>
      Grow Day: %GrowDay%<br><br>
      Total Days In Veg: %VegDays%<br><br>
      Total Days In Flower: %FlowerDays%<br><br>
      Hour: %GrowHour%<br><br>
      Watering in %WaterNXT% Hours<br><br>
      Water Tank Level: %tankLvl% Ounces Remaining<br><br>
      Device MAC: %mac%<br>
      </div>
      <div>Sensor Data<br><br>
       Tempature: %temp%<br><br>
       Humidity: %hum%<br><br>
       Soil Moisture: %moist%
      </div>
    </aside>
  </section>
  <footer>
    <p>Designed by Autobud.io</p>
  </footer>
</body>
<script>
  function PumpControl() {
    var phr = new XMLHttpRequest();
    phr.open('GET', "/PUMPON", true);
    phr.send();
  }
  function refill() {
    var phr = new XMLHttpRequest();
    phr.open('GET', "/refill", true);
    phr.send();
  }
  function PullData() {
    var phr = new XMLHttpRequest();
    phr.open('GET', "/PullData", true);
    phr.send();
  }
  function start() {
    var phr = new XMLHttpRequest();
    phr.open('GET', "/start", true);
    phr.send();
  }
  function post() {
    var phr = new XMLHttpRequest();
    phr.open('GET', "/post", true);
    phr.send();
  }
  function submitMessage() {
    alert("Value Saved");
    setTimeout(function(){ document.location.reload(false); }, 500);  
  }
</script>  
</html>)rawliteral";

// --- SPIFFS Helper Functions ---
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  String fileContent;
  while(file.available()){
    fileContent += String((char)file.read());
  }
  file.close();
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

// --- Configuration Load Function ---
void loadConfiguration() {
  String tempStr;

  tempStr = readFile(SPIFFS, "/ssid.txt");
  if (tempStr.length() > 0) wifiSSID = tempStr;

  tempStr = readFile(SPIFFS, "/wifipasswd.txt");
  if (tempStr.length() > 0) wifiPassword = tempStr;
  
  tempStr = readFile(SPIFFS, "/apiaddress.txt");
  if (tempStr.length() > 0) apiAddress = tempStr;

  tempStr = readFile(SPIFFS, "/alertaddress.txt");
  if (tempStr.length() > 0) alertAddress = tempStr;

  tempStr = readFile(SPIFFS, "/id.txt");
  if (tempStr.length() > 0) deviceID = tempStr;

  tempStr = readFile(SPIFFS, "/pumpgpio.txt");
  if (tempStr.length() > 0) pump_gpio = tempStr.toInt();

  tempStr = readFile(SPIFFS, "/ledgpio.txt");
  if (tempStr.length() > 0) led_gpio = tempStr.toInt();

  tempStr = readFile(SPIFFS, "/inputInt.txt");
  if (tempStr.length() > 0) waterInterval = tempStr.toInt();

  tempStr = readFile(SPIFFS, "/watertime.txt");
  if (tempStr.length() > 0) waterAmount = tempStr.toInt();

  tempStr = readFile(SPIFFS, "/watercalc.txt");
  if (tempStr.length() > 0) waterCal = tempStr.toFloat();

  tempStr = readFile(SPIFFS, "/volume.txt");
  if (tempStr.length() > 0) waterTankVolume = tempStr.toInt();

  tempStr = readFile(SPIFFS, "/threashold.txt");
  if (tempStr.length() > 0) dryThreshold = tempStr.toInt();
  
  tempStr = readFile(SPIFFS, "/lightInt.txt");
  if (tempStr.length() > 0) lightHours = tempStr.toInt();

  // Load State
  tempStr = readFile(SPIFFS, "/growlog.txt");
  if (tempStr.length() > 0) growDay = tempStr.toInt(); else growDay = 1;

  tempStr = readFile(SPIFFS, "/hour.txt");
  if (tempStr.length() > 0) hourCount = tempStr.toInt(); else hourCount = 1;

  tempStr = readFile(SPIFFS, "/VegDay.txt");
  if (tempStr.length() > 0) vegDays = tempStr.toInt(); else vegDays = 0;

  tempStr = readFile(SPIFFS, "/FlowerDay.txt");
  if (tempStr.length() > 0) flowerDays = tempStr.toInt(); else flowerDays = 0;

  tempStr = readFile(SPIFFS, "/waternext.txt");
  if (tempStr.length() > 0) waterNextHour = tempStr.toInt(); else waterNextHour = 0;
  
  tempStr = readFile(SPIFFS, "/H20tank.txt");
  if (tempStr.length() > 0) waterTankLevel = tempStr.toFloat(); else waterTankLevel = waterTankVolume;
  
  Serial.println("Configuration and state loaded.");
}

// --- WebUI Value Processor ---
String processor(const String& var){
  if(var == "inputInt"){ return String(waterInterval); }
  else if(var == "waterInt"){ return String(waterAmount); }
  else if(var == "volume"){ return String(waterTankVolume); }
  else if(var == "watercal"){ return String(waterCal); }
  else if(var == "threashold"){ return String(dryThreshold); }
  else if(var == "WiFiSSID"){ return wifiSSID; }
  else if(var == "lightInt"){ return String(lightHours); }
  else if(var == "mac"){ return macAddress; }
  else if(var == "WiFiPassword"){ return "********"; }
  else if(var == "GrowDay"){ return String(growDay); }
  else if(var == "GrowHour"){ return String(hourCount); }
  else if(var == "VegDays"){ return String(vegDays); }
  else if(var == "FlowerDays"){ return String(flowerDays); }
  else if(var == "devid"){ return deviceID; }
  else if(var == "pumpgpio"){ return String(pump_gpio); }
  else if(var == "ledgpio"){ return String(led_gpio); }
  else if(var == "WaterNXT"){ return String(waterNextHour); }
  else if(var == "tankLvl"){ return String(waterTankLevel); }
  else if(var == "temp"){ return String(temp); }
  else if(var == "moist"){ return String(moist); }
  else if(var == "hum"){ return String(hum); }
  else if(var == "apiaddress"){ return apiAddress; }
  else if(var == "alertadd"){ return alertAddress; }
  return String();
}

// --- WiFi Functions ---
void startAccessPoint() {
  Serial.println("Starting Access Point (AP) mode.");
  IPAddress apIP(192, 168, 4, 1); 
  
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); 
  WiFi.softAP(AP_SSID, AP_PASS);
  
  Serial.print("AP IP address: ");
  Serial.println(apIP); 
  localonly = true;

  dnsServer.start(53, "*", apIP); 
  Serial.println("DNS server started.");
}

void connectToWiFi() {
  bool forceAPMode = false;
  
  pinMode(RESET_BUTTON_GPIO, INPUT_PULLUP);
  Serial.print("Checking for reset button press... ");
  for(int i = 0; i < 30; i++) {
    if(digitalRead(RESET_BUTTON_GPIO) == LOW) {
      forceAPMode = true;
      break;
    }
    delay(100);
  }

  if (forceAPMode) {
    Serial.println("Button pressed, forcing AP mode.");
    startAccessPoint();
    return;
  }

  if (wifiSSID.length() == 0) {
    Serial.println("No WiFi SSID configured.");
    startAccessPoint();
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  Serial.print("Attempting to connect to WiFi: ");
  Serial.println(wifiSSID);

  int connect_timeout = 20; 
  while (WiFi.status() != WL_CONNECTED && connect_timeout > 0) {
    delay(500);
    Serial.print(".");
    connect_timeout--;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    macAddress = WiFi.macAddress();
    localonly = false;
    String abip = WiFi.localIP().toString();
    bootemail("Autobud Online! Manage at http://" + abip);
  } else {
    Serial.println("\nFailed to connect to WiFi.");
    startAccessPoint();
  }
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    
    int connect_timeout = 20; 
    while (WiFi.status() != WL_CONNECTED && connect_timeout > 0) {
      delay(500);
      connect_timeout--;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Reconnect failed. Restarting.");
      ESP.restart(); 
    } else {
      Serial.println("Reconnect successful.");
    }
  }
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  Serial.println("\nBooting Autobud...");

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  } else {
    Serial.println("SPIFFS mounted successfully.");
  }

  loadConfiguration();

  pinMode(pump_gpio, OUTPUT);
  digitalWrite(pump_gpio, LOW);
  pinMode(led_gpio, OUTPUT);
  digitalWrite(led_gpio, LOW);
  
  updateLighting(); 

  connectToWiFi(); 

  // --- Web Server Endpoints ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/PUMPON", HTTP_GET, [](AsyncWebServerRequest * request) {
    waterRequestFlag = true;
    request->send_P(200, "text/plain", "Turning on Pump");
  });

  server.on("/refill", HTTP_GET, [](AsyncWebServerRequest * request) {
    waterTankLevel = waterTankVolume;
    writeFile(SPIFFS, "/H20tank.txt", String(waterTankLevel).c_str());
    request->send_P(200, "text/plain", "Refilling Tank");
  });

  server.on("/PullData", HTTP_GET, [](AsyncWebServerRequest * request) {
    dataRequestFlag = true;
    request->send_P(200, "text/plain", "Pulling Readings from Meter");
  });

  server.on("/start", HTTP_GET, [](AsyncWebServerRequest * request) {
    growDay = 1;
    hourCount = 1;
    waterNextHour = 0;
    vegDays = 0;
    flowerDays = 0;
    triggerDryWater = true;
    
    writeFile(SPIFFS, "/growlog.txt", "1");
    writeFile(SPIFFS, "/hour.txt", "1");
    writeFile(SPIFFS, "/waternext.txt", "0");
    writeFile(SPIFFS, "/VegDay.txt", "0");
    writeFile(SPIFFS, "/FlowerDay.txt", "0");
    
    request->send_P(200, "text/plain", "Starting New Grow");
  });

  server.on("/post", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", "Rebooting");
    delay(1000);
    ESP.restart();
  });

  // --- Main Configuration Handler (/get) ---
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    bool configChanged = false;

    if (request->hasParam(PARAM_INT)) {
      inputMessage = request->getParam(PARAM_INT)->value();
      waterInterval = inputMessage.toInt();
      writeFile(SPIFFS, "/inputInt.txt", inputMessage.c_str());
      WaterUpdate();
      configChanged = true;
    }
    else if (request->hasParam(PARAM_WATERTM)) {
      inputMessage = request->getParam(PARAM_WATERTM)->value();
      waterAmount = inputMessage.toInt();
      writeFile(SPIFFS, "/watertime.txt", inputMessage.c_str());
      configChanged = true; // <-- THIS IS THE FIXED LINE
    }
    else if (request->hasParam(PARAM_VOLUME)) {
      inputMessage = request->getParam(PARAM_VOLUME)->value();
      waterTankVolume = inputMessage.toInt();
      writeFile(SPIFFS, "/volume.txt", inputMessage.c_str());
      configChanged = true;
    }
    else if (request->hasParam(PARAM_THREASHOLD)) {
      inputMessage = request->getParam(PARAM_THREASHOLD)->value();
      dryThreshold = inputMessage.toInt();
      writeFile(SPIFFS, "/threashold.txt", inputMessage.c_str());
      configChanged = true;
    }
    else if (request->hasParam(PARAM_WIFI_SSID)) {
      inputMessage = request->getParam(PARAM_WIFI_SSID)->value();
      wifiSSID = inputMessage;
      writeFile(SPIFFS, "/ssid.txt", inputMessage.c_str());
      configChanged = true;
    }
    else if (request->hasParam(PARAM_WIFI_PASS)) {
      inputMessage = request->getParam(PARAM_WIFI_PASS)->value();
      wifiPassword = inputMessage;
      writeFile(SPIFFS, "/wifipasswd.txt", inputMessage.c_str());
      configChanged = true;
    }
    else if (request->hasParam(PARAM_LIGHT)) {
      inputMessage = request->getParam(PARAM_LIGHT)->value();
      lightHours = inputMessage.toInt();
      writeFile(SPIFFS, "/lightInt.txt", inputMessage.c_str());
      updateLighting();
      configChanged = true;
    }
    else if (request->hasParam(PARAM_LEDOFFSET)) {
      inputMessage = request->getParam(PARAM_LEDOFFSET)->value();
      hourCount = inputMessage.toInt();
      writeFile(SPIFFS, "/hour.txt", inputMessage.c_str());
      updateLighting();
      configChanged = true;
    }
    else if (request->hasParam(PARAM_DAYADVANCE)) {
      inputMessage = request->getParam(PARAM_DAYADVANCE)->value();
      growDay = inputMessage.toInt();
      writeFile(SPIFFS, "/growlog.txt", inputMessage.c_str());
      configChanged = true;
    }
    else if (request->hasParam(PARAM_TEMP)) {
      temp = request->getParam(PARAM_TEMP)->value().toFloat();
    }
    else if (request->hasParam(PARAM_HUM)) {
      hum = request->getParam(PARAM_HUM)->value().toFloat();
    }
    else if (request->hasParam(PARAM_MOIST)) {
      moist = request->getParam(PARAM_MOIST)->value().toFloat();
    }
    else if (request->hasParam(PARAM_APIADDRESS)) {
      inputMessage = request->getParam(PARAM_APIADDRESS)->value();
      apiAddress = inputMessage;
      writeFile(SPIFFS, "/apiaddress.txt", inputMessage.c_str());
      configChanged = true;
    }
    else if (request->hasParam(PARAM_ALERT)) {
      inputMessage = request->getParam(PARAM_ALERT)->value();
      alertAddress = inputMessage;
      writeFile(SPIFFS, "/alertaddress.txt", inputMessage.c_str());
      configChanged = true;
    }
    else if (request->hasParam(PARAM_WATERCAL)) {
      inputMessage = request->getParam(PARAM_WATERCAL)->value();
      waterCal = inputMessage.toFloat();
      writeFile(SPIFFS, "/watercalc.txt", inputMessage.c_str());
      configChanged = true;
    }
    else if (request->hasParam(PARAM_DEVID)) {
      inputMessage = request->getParam(PARAM_DEVID)->value();
      deviceID = inputMessage;
      writeFile(SPIFFS, "/id.txt", inputMessage.c_str());
      configChanged = true;
    }
    else if (request->hasParam(PARAM_PUMPGPIO)) {
      inputMessage = request->getParam(PARAM_PUMPGPIO)->value();
      pump_gpio = inputMessage.toInt();
      writeFile(SPIFFS, "/pumpgpio.txt", inputMessage.c_str());
      pinMode(pump_gpio, OUTPUT);
      digitalWrite(pump_gpio, LOW);
      configChanged = true;
    }
    else if (request->hasParam(PARAM_LEDGPIO)) {
      inputMessage = request->getParam(PARAM_LEDGPIO)->value();
      led_gpio = inputMessage.toInt();
      writeFile(SPIFFS, "/ledgpio.txt", inputMessage.c_str());
      pinMode(led_gpio, OUTPUT);
      updateLighting();
      configChanged = true;
    }
    else {
      inputMessage = "No message sent";
    }
    
    if (configChanged) {
      Serial.println("Configuration saved: " + inputMessage);
    }
    
    request->send(200, "text/text", inputMessage);
  });

  // --- CAPTIVE PORTAL FIX ---
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.println("Handling 404/Captive Portal request.");
    request->send_P(200, "text/html", index_html, processor);
  });
  
  AsyncElegantOTA.begin(&server); 
  server.begin();
  Serial.println("Web server started.");
}

// --- Main Loop ---
void loop() {
  AsyncElegantOTA.loop();

  if (localonly) {
    dnsServer.processNextRequest(); 
  } else {
    checkWiFiConnection(); 
  }
  
  if (waterRequestFlag) {
    waterRequestFlag = false; 
    PUMPON1();
  }

  if (dataRequestFlag) {
    dataRequestFlag = false; 
    pullmeterdata();
  }

  if ((millis() - lastTime) > timerDelay) {
    lastTime = millis();
    handleHourlyTasks();
  }
}

// --- Core Logic Functions ---
void handleHourlyTasks() {
  hourCount++;
  
  if (hourCount > 24){
    hourCount = 1;
    handleDailyTasks();
  }

  writeFile(SPIFFS, "/hour.txt", String(hourCount).c_str());
  Serial.printf("Hour: %d\n", hourCount);
  updateLighting();

  if (!localonly) {
    HeartBeat();
  }

  if (triggerDryWater) {
    MoistureWater();
  }
  AutoWater();
}

void handleDailyTasks() {
  Serial.println("--- New Day Rollover ---");
  growDay++;
  writeFile(SPIFFS, "/growlog.txt", String(growDay).c_str());

  if (lightHours <= 12) {
    flowerDays++;
    writeFile(SPIFFS, "/FlowerDay.txt", String(flowerDays).c_str());    
  } else {
    vegDays++;
    writeFile(SPIFFS, "/VegDay.txt", String(vegDays).c_str());
  }
  
  Serial.printf("Grow Day: %d (Veg: %d, Flower: %d)\n", growDay, vegDays, flowerDays);

  if (!localonly) {
    HTTPClient http;
    String apipath1 = apiAddress + serverName;
    http.begin(wifiClient, apipath1);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String devip = WiFi.localIP().toString();
    String postdata = "Id=" + deviceID + "&GrowDay=" + growDay + "&Ip=" + devip;
    int httpResponseCode = http.POST(postdata);
    Serial.printf("Daily POST Response code: %d\n", httpResponseCode);
    http.end();
    
    DayCounter();
  }
  
  triggerDryWater = true;
  LowWaterCheck();
}

void updateLighting() {
  if (hourCount < lightHours) {
    if(digitalRead(led_gpio) == LOW) {
      Serial.println("LED ON");
      digitalWrite(led_gpio, HIGH);
    }
  } else {
    if(digitalRead(led_gpio) == HIGH) {
      Serial.println("LED OFF");
      digitalWrite(led_gpio, LOW);
    }
  }
}

void AutoWater() {
  if (waterNextHour == 0) {
    Serial.println("AutoWater: Time to water.");
    PUMPON1();
    WaterUpdate();
  } else {
    waterNextHour--;
    Serial.printf("AutoWater: %d hours until next water.\n", waterNextHour);
  }
  writeFile(SPIFFS, "/waternext.txt", String(waterNextHour).c_str());
}

void WaterUpdate(void) {
  Serial.println("Updating auto water frequency");
  waterNextHour = waterInterval;
}

void PUMPON1(void) {
  float calcedwatertimeX = (waterAmount * (waterCal * 1000.0));
  int waterTimeMillis = (int)calcedwatertimeX;

  Serial.printf("Watering for %d ms (%d oz @ %.2f s/oz)\n", waterTimeMillis, waterAmount, waterCal);
  
  digitalWrite(pump_gpio, HIGH);
  delay(waterTimeMillis);
  digitalWrite(pump_gpio, LOW);
  
  Serial.println("Watering Completed.");

  waterTankLevel = waterTankLevel - waterAmount;
  writeFile(SPIFFS, "/H20tank.txt", String(waterTankLevel).c_str());  

  if (!localonly) {
    HTTPClient http;
    String logpath1 = apiAddress + logapi;
    http.begin(wifiClient, logpath1);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String postdata = "Id=" + deviceID + "&msg=3&amt=" + String(waterAmount);
    Serial.println("Logging water event: " + postdata);
    int httpResponseCode = http.POST(postdata);
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    http.end();
  }

  triggerDryWater = false;
  LowWaterCheck();
}

void MoistureWater() {
  if ((moist > 0) && (moist < dryThreshold)) {
    Serial.print("SOIL DRY, setting water flag.");
    waterRequestFlag = true;
  }
}

void LowWaterCheck(void) {
  float percentleft = (waterTankLevel / waterTankVolume);
  if (percentleft < 0.25) { 
    Serial.println("WATER LOW: Sending email alert.");
    if (!localonly) {
      bootemail("Autobud Alert! H2O Reserve Low");
    }
  } else {
    Serial.printf("Water Tank Level: %.1f%%\n", percentleft * 100);
  }
}

// --- HTTP POST Functions ---
String HeartBeat() {
  HTTPClient http;
  String apipath1 = apiAddress + serverName;
  http.begin(wifiClient, apipath1);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String devip = WiFi.localIP().toString();
  String postdata = "Id=" + deviceID + "&GrowDay=" + growDay + "&Ip=" + devip;
  
  int httpResponseCode = http.POST(postdata);
  if(httpResponseCode != 200) {
    Serial.printf("HeartBeat HTTP error: %d\n", httpResponseCode);
  }
  http.end();
  return String(httpResponseCode);
}

void DayCounter(void) {
  HTTPClient http;
  String apipath1 = apiAddress + daycount;
  http.begin(wifiClient, apipath1);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String postdata = "Id=" + deviceID + "&veg=" + vegDays + "&flower=" + flowerDays;
  int httpResponseCode = http.POST(postdata);
  Serial.printf("DayCounter POST Response code: %d\n", httpResponseCode);
  http.end();
}

void bootemail(String emailmsg) {
  if (alertAddress.length() == 0) {
    Serial.println("No alert email address configured.");
    return;
  }

  HTTPClient http;
  String alertpath1 = apiAddress + alertapi;
  http.begin(wifiClient, alertpath1);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String postdata = "Id=" + deviceID + "&to=" + alertAddress + "&ip=" + emailmsg;
  Serial.println("Sending Email: " + postdata);
  int httpResponseCode = http.POST(postdata);
  Serial.printf("Email POST Response code: %d\n", httpResponseCode);
  http.end();
}

// --- Other Functions ---
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void pullmeterdata(void) {
  Serial.println("Data pull requested (not implemented).");
}
