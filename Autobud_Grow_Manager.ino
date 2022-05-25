#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "Arduino.h"
#include <StringArray.h>
#include <AsyncElegantOTA.h>
//ESP8266
#include <FS.h>
#include <ESPAsyncTCP.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

//This will be selectable in a new release
int resetbtn = 4;

AsyncWebServer server(80);

//User Entered WiFi Creds
const char* WiFiSSID = "WiFiSSID";
const char* WiFiPassword = "WiFiPassword";
const char* ssidd = "Autobud";
const char* passwordd = "Grow4Life";

//Global Var declarations
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

//Paths to API endpoint to post data
const char* serverName = "/upload";
const char* daycount = "/daycount";
const char* imgapi = "/imgupload";
const char* logapi = "/log";
const char* alertapi = "/emailalert";

int hourcount = 0;
int VegDays = 0;
int FullAutoH2en = 0;
int FlowerDays = 0;
int WaterRequest = 0;
int buttonState = 0;
int localonly = 0;
int DataRequest = 0;
int TriggerWater = 0;
String watertime = "0";


unsigned long lastTime = 0;
unsigned long timerDelay = 3600000;
//unsigned long timerDelay = 10000;
boolean PUMPON = false;
bool wifisetup = false;
int lighttime = 0;
int hour12 = 0;
int bloom = 0;
int growday = 0;
int LT = 0;
int LEDstat = 0;
int FD = 0;
int VD = 0;
int tankLvl = 0;
int autowater = 0;


//Default values when using external sensor
float temp = 0.0;
float hum = 0.0;
float moist = 0.0;
int wateroffset = 3; 
const int serverPort1 = 80;
String emailmsg = "";
WiFiClient client;



// HTML emebed
const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE HTML>
<style type="text/css">
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

body,html {
  background-color: #FFF;
}
header {
    background: #b1b6b8;
    padding: 20px 30px;
    max-width: 940px;
    margin: auto;
}
nav {
    display: inline-block;
}
nav ul li a {
    background: #EEE;
    padding: 2px 10px;
    font-size: 20px;
    text-decoration: none;
    font-weight: bold;
    color: #444;
    border-radius: 10px;
}
nav ul li a:hover {
  background: #FFF;
}
nav ul li {
    display: inline-block;
    margin: 10px;
}
nav ul {
    list-style: none;
}
#pageContent {
    max-width: 1000px;
    margin: auto;
    border: none;
}
main {
  float: left;
  width: 60%;
}
aside {
  float: right;
  width: 30%;
}
article {
    border-bottom: 2px dotted #999;
    padding-bottom: 20px;
    margin-bottom: 20px;
}
article h2 {
    font-weight: normal;
    margin-bottom: 12px;
}
article p {
  
}
main section {
  
}
footer {
  background: #b1b6b8;
  max-width: 1000px;
  margin: auto;
  clear: both;
  text-align: right;
}
footer p {
  padding: 20px;
}
address {
    padding: 10px 20px 30px 10px;
}
aside > div {
  margin: 10px auto;
  min-height: 100px;
}
#logo {
    display: inline-block;
    vertical-align: middle;
    font-size: 30px;
    margin-right: 75px;
    color: #FFF;
}
#logo img {
    width: 50px;
    vertical-align: middle;
    margin-right: 10px;
}
body > section {
    max-width: 1000px;
    margin: auto;
    padding: 30px 0px;
    border-bottom: 1px solid #999;
    color: #333;
}

@media screen and (max-width: 1000px) {
  body > section {
    padding: 30px 20px;
  }
}
@media screen and (max-width: 600px) {
  main {
    float: none;
    width: 100%;
  }
  aside {
    float: none;
    width: 100%;
  }

}
        </style>

  <html lang="en" class="no-js">
  </head>
    <meta charset="utf-8">
    <meta http-equiv="x-ua-compatible" content="ie=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Autobud</title>
    <link rel="stylesheet" href="style.css">
    <script src="script.js"></script>
</head>

<body>
  <header>
    <div id="logo"><img src="/logo.png">Autobud&nbsp;GrowManager</div>
    <nav>  
      <ul>
        <li><a href="/">Home</a>
        <li><a href="/update">Update Firmware</a>
        <li><a href="/post">Reboot</a>
        <li><a href="/start">Restart Grow</a>
      </ul>
    </nav>
  </header>
  <section>
    <strong>Beta Version: 0.961</strong>
  </section>
  <section id="pageContent">
    <main role="main">
      <article>
        <h2>Auto Water</h2>
        <p>
    </form><br>
   <form action="/get" target="_top">
    Auto Water: (Water Every %inputInt% Hours): <input type="number " name="inputInt">
    <input type="submit" value="Submit" onclick="submitMessage()">
    </form><br>
    <form action="/get" target="hidden-form">
    Water Amount (oz): (Current: %waterInt%): <input type="number " name="waterInt">
    <input type="submit" value="Submit" onclick="submitMessage()">
     </form><br>
    <form action="/get" target="hidden-form">
    Water Calibration (%watercal% seconds per oz): <input type="number " name="watercal">
    <input type="submit" value="Submit" onclick="submitMessage()">
    </form><br>
    <form action="/get" target="hidden-form">
    Water Tank Volume ( %volume% oz): <input type="number " name="volume">
    <input type="submit" value="Submit" onclick="submitMessage()">
    </form><br>
    <form action="/get" target="hidden-form">
    Dry Soil Threashold ( %threashold% Precent Dry): <input type="number " name="threashold">
    <input type="submit" value="Submit" onclick="submitMessage()">
    
        </p>
      </article>
      <article>
        <h2>Light Recording</h2>
        </form><br>
        <form action="/get" target="hidden-form">
          Light Hours: (Current: %lightInt%): <input type="number " name="lightInt">
          <input type="submit" value="Submit" onclick="submitMessage()">
         </form><br>
        </p>
      </article>
      <article>
        <h2>Auto Override</h2>
        <p>
          <button onclick="PumpControl();">Water</button>
          <button onclick="refill();">Refill Water Tank</button>
          </form><br>
         <form action="/get" target="hidden-form">
          Grow Hour Shift: <input type="number " name="ledoffset">
          <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
         <form action="/get" target="hidden-form">
          Grow Day Shift: <input type="number " name="dayadvance">
          <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
            
        </p>
      </article>
      <article>
        <h2>Connections & Configuration</h2>
        <p>   </form><br>
        <form action="/get" target="hidden-form">
         WiFi SSID (%WiFiSSID%): <input type="text" name="WiFiSSID">
         <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
        <form action="/get" target="hidden-form">
         WiFi Password (****): <input type="text" name="WiFiPassword">
         <input type="submit" value="Submit" onclick="submitMessage()">
         </form><br></p>
          <form action="/get" target="hidden-form">
         API Endpoint: (Current: %apiaddress%): <input type="number " name="apiaddress">
         <input type="submit" value="Submit" onclick="submitMessage()">
         </form><br></p>
          <form action="/get" target="hidden-form">
         Alert Text/Email: (Current: %alertadd%): <input type="number " name="alertadd">
         <input type="submit" value="Submit" onclick="submitMessage()">
         </form><br></p>
         <form action="/get" target="hidden-form">
         Device ID: (Current: %devid%): <input type="text" name="devid">
         <input type="submit" value="Submit" onclick="submitMessage()">
         </form><br></p>
         <form action="/get" target="hidden-form">
         Pump GPIO: (Current: %pumpgpio%): <input type="text" name="pumpgpio">
         <input type="submit" value="Submit" onclick="submitMessage()">
         </form><br></p>
      </article>
    </main>
    <aside>
      <div>Grow Status
      </form><br>
    </form>
    DeviceID: %devid%
    </form><br>
    Grow Day: %GrowDay%
    </form><br>
    Total Days In Veg: %VegDays%
    </form><br>
    Total Days In Flower: %FlowerDays%
    </form><br>
    Hour: %GrowHour%
    </form><br>
    Watering in %WaterNXT% Hours
    </form><br>
    Water Tank Level: %tankLvl% Ounces Remaining
  </div>
      <div>Sensor Data
      </form><br>
      Tempature: %temp%
      </form><br>
      Humidity: %hum%
      </form><br>
      Soil Moisture: %moist%
      </div>
      <div> 
      </div>
    </aside>
  </section>
  <footer>
    <p>Designed by Autobud.io</a></p>
  </footer>


</body> 


  
<script>
  function PumpControl() {
    var phr = new XMLHttpRequest();
    phr.open('GET', "/PUMPON", true);
    phr.send();
  }

  function H2Auto() {
    var phr = new XMLHttpRequest();
    phr.open('GET', "/Ht2Auto", true);
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

 
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// SIFFS R/W operations
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  Serial.println(fileContent);
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
}

//Defines grow day
String sDAY = readFile(SPIFFS, "/growlog.txt");
int DAY = sDAY.toInt();

//Triggers the next update of a water event. Used in water shceduling (ie. Water X amount every Y Days)
void WaterUpdate( void ) {
  Serial.println("Updating auto water frequency");
  int waternext;

//  int growdayrd = readFile(SPIFFS, "/growlog.txt").toInt();
  int waterfreq = readFile(SPIFFS, "/inputInt.txt").toInt();
  waternext = waterfreq;
//
//  waternext = (waterfreq + growdayrd);
   String waternextSTR = String(waternext);
    writeFile(SPIFFS, "/waternext.txt", waternextSTR.c_str());
//  Serial.println("Water Update Complete. Next water day = ");
//  Serial.println(waternext);
}

//Stuff for outputting local vars in the web UI
String processor(const String& var){
  //Serial.println(var);
   if(var == "inputInt"){
    return readFile(SPIFFS, "/inputInt.txt");
  }
    else if(var == "waterInt"){
    return readFile(SPIFFS, "/watertime.txt");
  }
    else if(var == "volume"){
    return readFile(SPIFFS, "/volume.txt");
  }
    else if(var == "watercal"){
    return readFile(SPIFFS, "/watercalc.txt");
  }

    else if(var == "threashold"){
    return readFile(SPIFFS, "/threashold.txt");
  }
    else if(var == "WiFiSSID"){
    return readFile(SPIFFS, "/ssid.txt");
  }
    else if(var == "lightInt"){
    return readFile(SPIFFS, "/lightInt.txt");
    
  }

    else if(var == "WiFiPassword"){
    return readFile(SPIFFS, "/wifipasswd.txt");
  }
    else if(var == "GrowDay"){
    return readFile(SPIFFS, "/growlog.txt");
  }
    else if(var == "GrowHour"){
    return readFile(SPIFFS, "/hour.txt"); 
  }
    else if(var == "VegDays"){
    return readFile(SPIFFS, "/VegDay.txt"); 
    //return String(VegDays);
  }
    else if(var == "FlowerDays"){
    return readFile(SPIFFS, "/FlowerDay.txt");
    //return String(FlowerDays);
  }
    else if(var == "WaterRequest"){
    return String(WaterRequest);
  }
  else if(var == "devid"){
    //return String(ID);
    return readFile(SPIFFS, "/id.txt");   
  }
  else if(var == "pumpgpio"){
    return readFile(SPIFFS, "/pumpgpio.txt");   
  }
  else if(var == "WaterNXT"){
    return readFile(SPIFFS, "/waternext.txt");
  }
  else if(var == "tankLvl"){
    return readFile(SPIFFS, "/H20tank.txt");
    //return calcH20S;
  }
  else if(var == "temp"){
    return String(temp);
  }
  else if(var == "moist"){
    return String(moist);
  }
 else if(var == "hum"){
    return String(hum);
  }
else if(var == "abaddress"){
    return readFile(SPIFFS, "/abaddress.txt");
}

else if(var == "abcamaddress"){
    return readFile(SPIFFS, "/abcamaddress.txt");
}

else if(var == "apiaddress"){
    return readFile(SPIFFS, "/apiaddress.txt");
}

else if(var == "alertadd"){
    return readFile(SPIFFS, "/alertaddress.txt");
}
  
  return String();
}

void setup() {
  Serial.begin(115200);
//SPIFFS must be mounted before anything can happen
if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");  
    
    
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }



  // Check reset button (if user pushes then goes into reset mode)
  pinMode(resetbtn, INPUT);

  //Check for button push for 10 seconds
  int btncheck = 10;
  //bool wifisetup = false;

  //If the button is pushed goes into setup mode
  while (btncheck > 0) {
    buttonState = digitalRead(resetbtn);
    if (buttonState == LOW) {
    localWifi();

 
  } 
  else {
    // turn LED off:
    Serial.println("No Push");
    wifisetup = false;
  }
  btncheck = (btncheck - 1);
  Serial.println(btncheck);
  delay(1000);
   
  }
Serial.print(wifisetup);
// pull saved value from SPIFFs
String savedssid = readFile(SPIFFS, "/ssid.txt");
String savedpass = readFile(SPIFFS, "/wifipasswd.txt");
int ssidlength = savedssid.length();
Serial.print("SSID Length:");
Serial.print(ssidlength);


//if the SSID is blank, then revert to local mode (or if it cant connect for whatever reason)
if (ssidlength == 0) {
  wifisetup == true;
  localWifi();
}


 //If Wifi config was pressed, skip this, if not, run:
 if (wifisetup == false) {
   WiFi.mode(WIFI_STA);
   Serial.print("Using SSID ");
   Serial.println(savedssid);
   Serial.print("Using Password ");
   Serial.println(savedpass);
   //char hname[19];
   //snprintf(hname, 12, "ESP%d-LIGHT", 32);
   char hname[] = "AutobudGrowMGR";

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.print("Attempting to connect to WiFi ");
   WiFi.begin(savedssid.c_str(), savedpass.c_str());

    // wait 10 seconds for connection:
    delay(10000);
  }

  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  //Posts to the API server the device is online. From there an email is generated and sent telling the user of the IP
  String abip = WiFi.localIP().toString();
  emailmsg = "Autobud Online! Connect to your local WiFi network and Manage your device at http://" + abip;
  HeartBeat();
  bootemail();
  
  //end wifi
 }
//Setup all relays
//    pinMode(pump, OUTPUT);
//    digitalWrite(pump, LOW);
    

  //BootLEDCheck();

//Manual watering by button handeling
 server.on("/PUMPON", HTTP_GET, [](AsyncWebServerRequest * request) {
    WaterRequest = 1;
    //digitalWrite(pump, LOW);
    request->send_P(200, "text/plain", "Turning on Pump");
  });

//User initiates water tank refill from UI
  server.on("/refill", HTTP_GET, [](AsyncWebServerRequest * request) {
    //Check if the Tank config is empty 
    String refill;
    String tankvolume = readFile(SPIFFS, "/volume.txt");
     if (tankvolume == NULL){
     refill = 120;;
     } else {
     refill = tankvolume.toInt();
     }
    
    //String refill;
    //refill = 120;
    writeFile(SPIFFS, "/H20tank.txt", refill.c_str());
    request->send_P(200, "text/plain", "Refilling Tank");
  });


//UI button to pull data from the remote sensor. NOT ACTIVE RIGHT NOW
  server.on("/PullData", HTTP_GET, [](AsyncWebServerRequest * request) {
    DataRequest = 1;
    request->send_P(200, "text/plain", "Pulling Readings from Meter");
  });

    
// Button event to restart a grow. Sets everything to first grow state
  server.on("/start", HTTP_GET, [](AsyncWebServerRequest * request) {
    //Write growlog.txt to SPIFS
    String grow;
    grow = "1";
    writeFile(SPIFFS, "/growlog.txt", grow.c_str());
    //NEW Reset hour count as well
    String ResetHour;
    ResetHour = "1";
    writeFile(SPIFFS, "/hour.txt", ResetHour.c_str());
    hourcount = 1;
    String ResetWtr;
    ResetWtr = "0";
    writeFile(SPIFFS, "/waternext.txt", ResetWtr.c_str());
    String ResetDays;
    ResetDays = "0";
    writeFile(SPIFFS, "/VegDay.txt", ResetDays.c_str());
    writeFile(SPIFFS, "/FlowerDay.txt", ResetDays.c_str());
    TriggerWater = 1;
    
    request->send_P(200, "text/plain", "Starting New Grow");
  });

// Reboots the device
  server.on("/post", HTTP_GET, [](AsyncWebServerRequest * request) {
ESP.restart();

    request->send_P(200, "text/plain", "Rebooting");
  });

// Turns exhaust fan on or off 
  server.on("/fan", HTTP_GET, [](AsyncWebServerRequest * request) {
   //bootemail();
   

  TriggerWater = 0;
    request->send_P(200, "text/plain", "Fan Action");
  });

  // Send web page with input fields to client
  // This loads everything 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

//This is a simple program I use for OTA updates.

AsyncElegantOTA.begin(&server);    // Start ElegantOTA

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    if (request->hasParam(PARAM_INT)) {
      inputMessage = request->getParam(PARAM_INT)->value();
      writeFile(SPIFFS, "/inputInt.txt", inputMessage.c_str());
      WaterUpdate();
    }
    else if (request->hasParam(PARAM_WATERTM)) {
      inputMessage = request->getParam(PARAM_WATERTM)->value();
      writeFile(SPIFFS, "/watertime.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_VOLUME)) {
      inputMessage = request->getParam(PARAM_VOLUME)->value();
      writeFile(SPIFFS, "/volume.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_THREASHOLD)) {
      inputMessage = request->getParam(PARAM_THREASHOLD)->value();
      writeFile(SPIFFS, "/threashold.txt", inputMessage.c_str());
    }
    else if (request->hasParam(WiFiSSID)) {
      inputMessage = request->getParam(WiFiSSID)->value();
      writeFile(SPIFFS, "/ssid.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_LIGHT)) {
      inputMessage = request->getParam(PARAM_LIGHT)->value();
      writeFile(SPIFFS, "/lightInt.txt", inputMessage.c_str());
    }

    //This allows the user to change the time when the schedule starts or stops. This is a bit dirty but works.
    else if (request->hasParam(PARAM_LEDOFFSET)) {
      inputMessage = request->getParam(PARAM_LEDOFFSET)->value();
      int offset = inputMessage.toInt();
      //ledoffset = offset;
      String SyncHour = String(offset);
      writeFile(SPIFFS, "/hour.txt", SyncHour.c_str());
      //Update Var
      hourcount = offset;
    }
    // This allows for the user to change the grow day in the controller. 
    else if (request->hasParam(PARAM_DAYADVANCE)) {
      inputMessage = request->getParam(PARAM_DAYADVANCE)->value();
      int dayoffset = inputMessage.toInt();
      String DayMove = String(dayoffset);
      writeFile(SPIFFS, "/growlog.txt", DayMove.c_str());
    }
    
    else if (request->hasParam(WiFiPassword)) {
      inputMessage = request->getParam(WiFiPassword)->value();
      writeFile(SPIFFS, "/wifipasswd.txt", inputMessage.c_str());
    }

    //This is essentually a little web listener that listens for posts from the external water controller

    else if (request->hasParam(PARAM_TEMP)) {
      inputMessage = request->getParam(PARAM_TEMP)->value();
      temp = inputMessage.toFloat();
    }
     else if (request->hasParam(PARAM_HUM)) {
      inputMessage = request->getParam(PARAM_HUM)->value();
      hum = inputMessage.toFloat();
    }
    else if (request->hasParam(PARAM_MOIST)) {
      inputMessage = request->getParam(PARAM_MOIST)->value();
     moist = inputMessage.toFloat();
    }
    //External sensor IP. NOT USED RIGHT NOW. I point the external sensors at the IP of the controller right now.
    else if (request->hasParam(PARAM_ABADDESS)) {
      inputMessage = request->getParam(PARAM_ABADDESS)->value();
      writeFile(SPIFFS, "/abaddress.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_ABCAMADDESS)) {
      inputMessage = request->getParam(PARAM_ABCAMADDESS)->value();
      writeFile(SPIFFS, "/abcamaddress.txt", inputMessage.c_str());
    }
    //Allows users to point the data posts to whatever API they want.
    else if (request->hasParam(PARAM_APIADDRESS)) {
      inputMessage = request->getParam(PARAM_APIADDRESS)->value();
      writeFile(SPIFFS, "/apiaddress.txt", inputMessage.c_str());
    }
    //User added email address for alerts to go through. They are posted the my API which handles the email sending with passed
    //data from the controller
    else if (request->hasParam(PARAM_ALERT)) {
      inputMessage = request->getParam(PARAM_ALERT)->value();
      writeFile(SPIFFS, "/alertaddress.txt", inputMessage.c_str());
    }

    //This allows for self watering calibration. These are user generated tasks. Basically, you test the water system and write a value
    //here for how long it takes the system to deliver 1oz of water. All water times are built off this value.
    else if (request->hasParam(PARAM_WATERCAL)) {
      inputMessage = request->getParam(PARAM_WATERCAL)->value();
      writeFile(SPIFFS, "/watercalc.txt", inputMessage.c_str());
    }

    else if (request->hasParam(PARAM_DEVID)) {
      inputMessage = request->getParam(PARAM_DEVID)->value();
      writeFile(SPIFFS, "/id.txt", inputMessage.c_str());
    }

    else if (request->hasParam(PARAM_PUMPGPIO)) {
      inputMessage = request->getParam(PARAM_PUMPGPIO)->value();
      writeFile(SPIFFS, "/pumpgpio.txt", inputMessage.c_str());
    }
    
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();
}


// Read Saved values from SPIFFS at boot and assign them to varriables
int GrowInfo = readFile(SPIFFS, "/growlog.txt").toInt();
int bloomstate = readFile(SPIFFS, "/bloom.txt").toInt();



void loop() {
AsyncElegantOTA.loop();

//These operations happen repeataly 


//Check if watering was called
  if (WaterRequest == 1) {
    PUMPON1();
  }


//Check if data was to be pulled
  if (DataRequest == 1) {
    pullmeterdata();
  }


   if (hourcount > 24){
//End of Day operations
// Step1: Advance the day
    GrowInfo = readFile(SPIFFS, "/growlog.txt").toInt();
    GrowInfo = (GrowInfo + 1);
    String StrGrowInfo = String(GrowInfo);
    writeFile(SPIFFS, "/growlog.txt", StrGrowInfo.c_str());

//Step 2: Record Daytype based on lighting. This way the system knows how many days were spent in flower and how many
// were spent in veg. Very useful for monitoring.
String ReadFileLT = readFile(SPIFFS, "/lightInt.txt");
  LT = ReadFileLT.toInt();

String ReadFileFlowerDay = readFile(SPIFFS, "/FlowerDay.txt");
  FD = ReadFileFlowerDay.toInt();

String ReadFileVegDay = readFile(SPIFFS, "/VegDay.txt");
  VD = ReadFileVegDay.toInt();
  

  if (LT <= 12){
    FD = (FD + 1);
    String StrFD = String(FD);
    writeFile(SPIFFS, "/FlowerDay.txt", StrFD.c_str());   
  }

  else {
    VD = (VD + 1);
    String StrVD = String(VD);
    writeFile(SPIFFS, "/VegDay.txt", StrVD.c_str());
  }

    
    hourcount = 1;
    Serial.print("Grow Day Updated, Day is:");
    Serial.print("");
    Serial.print(GrowInfo);
    Serial.print("");
    Serial.print("A New day Has begun, Setting Hour = 0");
    Serial.print(hourcount);
    String hourcounter = String(hourcount);
    writeFile(SPIFFS, "/hour.txt", hourcounter.c_str());
    
// Step 4: Post to API that a new day has begun
    //Setup API Path
    String apipath = readFile(SPIFFS, "/apiaddress.txt");
    String apipath1 = apipath + serverName;
    
    HTTPClient http;
    //http.begin(serverName);
    http.begin(apipath1);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String devip = WiFi.localIP().toString().c_str();
    String IDx = readFile(SPIFFS, "/id.txt"); 
    int growdayagain = readFile(SPIFFS, "/growlog.txt").toInt();
    String postdata = "Id=" + IDx + "&GrowDay=" + growdayagain + "&Ip=" + devip;
    int httpResponseCode = http.POST(postdata);
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    delay(1000);
    http.end();
    
    //Step 5: Checking Watering. Here we do diffrent things depending if auto is online
    // NEW RELEASE MOVING TO HOURLY FUNCTIONS  
//    WaterFormula1();
    
//    int waternext = readFile(SPIFFS, "/waternext.txt").toInt();
//
////Step 6 Execute water if it is a water day
//if (GrowInfo == waternext) {
//  //WATER
//  PUMPON1();
//  //INCRAMENT
//  WaterUpdate();
//  
// }

 
 //Step 8 Set the TriggerWater monitor back to 0 if soil drys out today. ONLY FOR EXTERNAL SENSOR
 TriggerWater = 1;

//STEP 9 Check water reserve and alert if low
LowWaterCheck();

// STEP 10 Post day counts 
DayCounter();

 //END NEW DAY ROLLOVER
 }
  
//Run Lighting
Lighting();
  
}
//END LOOP

//Function delcarations 

// Posts to the API once an hour that device is alive 
String HeartBeat() {
    int Test = readFile(SPIFFS, "/growlog.txt").toInt();
    String IDx = readFile(SPIFFS, "/id.txt"); 
    Serial.print("Grow Day is:  ");
    Serial.print(Test);
    Serial.print("Device ID is:  ");
    Serial.print(IDx);
    HTTPClient http;

    //Setup API Path
    String apipath = readFile(SPIFFS, "/apiaddress.txt");
    String apipath1 = apipath + serverName;
    
    //http.begin(serverName);
    http.begin(apipath1);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String devip = WiFi.localIP().toString().c_str();
    String postdata = "Id=" + IDx + "&GrowDay=" + Test + "&Ip=" + devip;
    int httpResponseCode = http.POST(postdata);
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    delay(15000);
    http.end();
}

// Here is how I handle lighting.
//
void Lighting ( void ){
  
if ((millis() - lastTime) > timerDelay) {
hourcount = readFile(SPIFFS, "/hour.txt").toInt();

  String ReadFileLT = readFile(SPIFFS, "/lightInt.txt");
  LT = ReadFileLT.toInt();
  Serial.println("Lighting Check: Light Time = ");
  Serial.println(LT);
  Serial.println("Current Hour = ");
  Serial.println(hourcount);
  if (hourcount < LT) {
      Serial.println("LED ON");
      LEDstat = 1;
      //digitalWrite(relay, HIGH);
}
 else {
     Serial.println("LED OFF");
     LEDstat = 0;
     //digitalWrite(relay, LOW);

 }

 if (hourcount <= 24){
  hourcount = (hourcount + 1);
  Serial.print("Grow Hour is:");
  Serial.print("");
  Serial.print(hourcount);
  
  //save to variable in case of reboot
  String hourcounter = String(hourcount);
  writeFile(SPIFFS, "/hour.txt", hourcounter.c_str());
 }
 
 lastTime = millis();
//Update to the API that the loop ran

if (localonly == 0){
   int alive = WiFi.status();
   String IDx = readFile(SPIFFS, "/id.txt"); 
   
    //This still doesnt work right, set to reboot if it can not get out ot the internet
   if (alive = 3){
   //if (WiFi.status() = WL_CONNECTED){
     Serial.println("Sending Heartbeat");
    HTTPClient http;
    //http.begin(logapi);

    String logpath = readFile(SPIFFS, "/apiaddress.txt");
    String logpath1 = logpath + logapi;
    
    http.begin(logpath1);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String postdata = "Id=" + IDx + "&msg=" + LEDstat + "&amt=" + 0;
    int httpResponseCode = http.POST(postdata);
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();
     
     
     Serial.println("Hreatbeat sent");
   }
   else {
    Serial.println("No Intrenet connection, rebooting");
    ESP.restart();
   }
}

//Hourly Checks go here. Not really used except if an external sensor is monitoring soil moisture.
//Here is where the controller can get really granular. It can water at the near perfect time when the soil
//gets a configued dry value. IE (Water X amount when the soil reachs Y% moisture)
//Check Soil Moisture
if (TriggerWater == 1) {
  Serial.print("Checking for soil moisture");
    MoistureWater();

}

//Check auto water
AutoWater();

//End Hourly Checks 
}

}

//Wifi Setup Opps
void localWifi ( void ){
    Serial.println("Local WiFi");
    WiFi.softAP(ssidd, passwordd) ;
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    server.begin();
    localonly = 1;
    wifisetup = true;
}

//Here is everything for claucalting and delivering accurate timed watering
void PUMPON1( void ) 
{

//Define Pump GPIO 
int pump = readFile(SPIFFS, "/pumpgpio.txt").toInt();
pinMode(pump, OUTPUT);
digitalWrite(pump, LOW);


// read the value from the spiffs
String watertimeN = readFile(SPIFFS, "/watertime.txt");
int calcedwatertime = watertimeN.toInt();

String wateroffsetString = readFile(SPIFFS, "/watercalc.txt");
//Make sure there is data here, if not use the default

if (wateroffsetString == NULL){
   wateroffset = 4;
} else {
//wateroffset = wateroffsetString.toInt();
wateroffset = wateroffsetString.toFloat();
}

//int calcedwatertimeX = (calcedwatertime * (wateroffset * 1000));
float calcedwatertimeX = (calcedwatertime * (wateroffset * 1000));

Serial.println("Watering for: ");
Serial.println(calcedwatertimeX);
  digitalWrite(pump, HIGH);
  delay(calcedwatertimeX);
  digitalWrite(pump, LOW);
  Serial.println("Watering Completed:");
  Serial.println(".................");
  Serial.println(calcedwatertimeX);

//Deduct water total from Tank
String h20TankLvl = readFile(SPIFFS, "/H20tank.txt");
int calch20TankLvl = h20TankLvl.toInt();
calch20TankLvl = (calch20TankLvl - calcedwatertime);
String H20Update;
H20Update = String(calch20TankLvl);
writeFile(SPIFFS, "/H20tank.txt", H20Update.c_str()); 
  

  //post to API what just happened. Super useful for building an auto water algo
  HTTPClient http;
  String IDx = readFile(SPIFFS, "/id.txt"); 
    //http.begin(logapi);
    String logpath = readFile(SPIFFS, "/apiaddress.txt");
    String logpath1 = logpath + logapi;
    
    http.begin(logpath1);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    //String devip = WiFi.localIP().toString().c_str();
    //String postdata = "Id=" + ID + "&msg=" + growday + "&Ip=" + devip;
    String postdata = "Id=" + IDx + "&msg=" + 3 + "&amt=" + watertimeN;
    Serial.println(postdata);
    int httpResponseCode = http.POST(postdata);
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();

    WaterRequest = 0;
    TriggerWater = 0;
    LowWaterCheck();

}


 void pullmeterdata( void )
 {
  DataRequest = 0;
  Serial.println("DATA RESET");
  //http.end();
 }

 void bootemail ( void )
 {
//post to API
  HTTPClient http;
    String IDx = readFile(SPIFFS, "/id.txt");
    String email = readFile(SPIFFS, "/alertaddress.txt");
    //emailmsg = "Autobud Online! Connect to your local WiFi network & Manage your device @ http://" + abip;
    String alertpath = readFile(SPIFFS, "/apiaddress.txt");
    String alertpath1 = alertpath + alertapi;
    //String alertpath1 = alertpath + logapi;
    
    Serial.println(alertpath1);
    
    //http.begin(alertapi);
    http.begin(alertpath1);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String postdata = "Id=" + IDx + "&to=" + email + "&ip=" + emailmsg;
    int httpResponseCode = http.POST(postdata);
    Serial.println(postdata);
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();

 }

//External sensor based watering
void MoistureWater ( void )
{
  String moistthreshST = readFile(SPIFFS, "/threashold.txt");
  float moistthresh;
  moistthresh = moistthreshST.toFloat();
  
  //if (moist > 0) {
    if ((moist > 0) and (moist < moistthresh)){
       Serial.print("SOIL DRY SETTING WATER FLAG ");
        WaterRequest = 1;
        Serial.print("WATER FLAG SET");
    }
      
   // } 
    //TriggerWater = 0;
    Serial.print("SOIL NOT DRY");
  }

  void LowWaterCheck ( void )
  {
    float TankVolumeF = readFile(SPIFFS, "/volume.txt").toFloat();
    float h20TankLvl = readFile(SPIFFS, "/H20tank.txt").toFloat();;

//float calch20TankLvF = h20TankLvl.toFloat();
float precentleft = (h20TankLvl / TankVolumeF);
if (precentleft < 0.25){
  //Notify 
  Serial.print("EMAILING WATER LOW");
  emailmsg = "Autobud Alert! H2O Reserve Low";
  bootemail();
}
}

void DayCounter ( void )
 {
//post to daycount API
    String apipath = readFile(SPIFFS, "/apiaddress.txt");
    String apipath1 = apipath + daycount;
    int flowerday = readFile(SPIFFS, "/FlowerDay.txt").toInt();
    int vegday = readFile(SPIFFS, "/VegDay.txt").toInt();
    
    HTTPClient http;
    http.begin(apipath1);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String IDx = readFile(SPIFFS, "/id.txt"); 
    String postdata = "Id=" + IDx + "&veg=" + vegday + "&flower=" + flowerday;
    int httpResponseCode = http.POST(postdata);
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    delay(1000);
    http.end();

 }

//WORKING HERE
 void AutoWater ( void )
 {

int waternext = readFile(SPIFFS, "/waternext.txt").toInt();
int waterfreq = readFile(SPIFFS, "/inputInt.txt").toInt();

if (waternext == 0) {
   PUMPON1();
   WaterUpdate();
   //waternext = waterfreq;
}
else {
 waternext = (waternext - 1);
 String waterstring = String(waternext);
 writeFile(SPIFFS, "/waternext.txt", waterstring.c_str());
}
} 
 

 
