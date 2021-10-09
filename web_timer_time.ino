#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <Hash.h>
#include <FS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#define RELAY 2
boolean RELAYstate = true;
unsigned long previousMillis, currentMillis, timerMillis;
int onTime, offTime, hour_now, min_now, hour_on, min_on, hour_off, min_off;
int timeOffset = 25200;
int interval = onTime;
//int hour_on = 11;
//int min_on = 23;
//int hour_off;
//int min_off;
String hour_on_Content, min_on_Content, hour_off_Content, min_off_Content, onContent, offContent, ssidContent, passContent, formattedDate, timeStamp, dayStamp, dayTime;
const char* PARAM_HOUR_ON = "input_hour_on";
const char* PARAM_MIN_ON = "input_min_on";
const char* PARAM_HOUR_OFF = "input_hour_off";
const char* PARAM_MIN_OFF = "input_min_off";
const char* PARAM_ON = "input_ON";
const char* PARAM_OFF = "input_OFF";
const char* PARAM_SSID = "input_ssid";
const char* PARAM_PASSWORD = "input_password";
char ssid[24], password[32];
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html lang="ru"><head><meta charset="UTF-8">
  <title>ESP Timer Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
  </style>
  </head><body>
  <h1>Главное меню железки</h1>
  <h2>Настройки WiFi</h2>
    <form action="/get">
    Введите точку доступа: <input type="text" name="input_ssid">
    <input type="submit" value="Тыкай!">
  </form><br>
  <form action="/get">
    Введите пароль:<input type="text" name="input_password">
    <input type="submit" value="Тыкай!">
  </form><br>
  <h2>Настройки циклического таймера</h2>
  <form action="/get">
    Время работы(в минутах): <input type="number" name="input_ON">
    <input type="submit" value="Тыкай!">
  </form><br>
  <form action="/get">
    Время простоя (в минутах): <input type="number" name="input_OFF">
    <input type="submit" value="Тыкай!">
  </form><br>
  <h2>Настройки таймера по времени</h2>
  <form action="/get">
    Время включения часы: <input type="number" name="input_hour_on">
    <input type="submit" value="Тыкай!">
  </form><br>
  <form action="/get">
    Время включения минуты: <input type="number" name="input_min_on">
    <input type="submit" value="Тыкай!">    
  </form><br>
  <form action="/get">
    Время выключения часы: <input type="number" name="input_hour_off">
    <input type="submit" value="Тыкай!">    
  </form><br>
  <form action="/get">
    Время выключения минуты: <input type="number" name="input_min_off">
    <input type="submit" value="Тыкай!">    
  </form><br>
  <p> 
  <span class="time"></span> <span id="time">%time_on%</span>
  </p> 
  <p> 
  <span class="time"></span> <span id="time">%time_off%</span>
  </p>
  <p> 
  <span class="time">Текущее время:</span> <span id="time">%time%</span>
  </p>
</body></html>)rawliteral";
AsyncWebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String processor(const String& var) {
  if (var == "input_ssid") {
    return readFile(SPIFFS, "/ssid");
  }
  else if (var == "time_on"){
    String time_ons = "Время включения: " + String(hour_on) + "." + String(min_on) + "\n";
    return time_ons;
  }
  else if (var == "time_off"){
    String time_offs = "Время выключения: " + String(hour_off) + "." + String(min_off) + "\n";
    return time_offs;
  }
  else if (var == "time") {
    getTime();
    return String(timeStamp);
  }
  else if (var == "input_password") {
    return readFile(SPIFFS, "/pass");
  }
  else if (var == "input_ON") {
    return readFile(SPIFFS, "/ontime");
  }
  else if (var == "input_OFF") {
    return readFile(SPIFFS, "/offtime");
  }
  else if (var == "input_hour_on") {
    return readFile(SPIFFS, "/hour_on");
  }
  else if (var == "input_min_on") {
    return readFile(SPIFFS, "/min_on");
  }
  else if (var == "input_hour_off") {
    return readFile(SPIFFS, "/hour_off");
  }
  else if (var == "input_min_off") {
    return readFile(SPIFFS, "/min_off");
  }
  return String();
}
void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("\nОшибка монтирования SPIFFS\n");
    return;
  }
  else {
    Serial.println("\nSPIFFS примонтирована");
    Serial.println("..............................................");
  }
}
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}
String readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while (file.available()) {
    fileContent += String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}
void initWifi() {
  const char* ssidAP = "Умный_дом";
  const char* passAP = "517500FeFe";
  IPAddress local_IP(192, 168, 1, 149);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 0, 0);
  IPAddress primaryDNS(8, 8, 8, 8);
  IPAddress secondaryDNS(8, 8, 4, 4);
  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
    Serial.println("..............................................");
    initTime();
    return;
  }
  else if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi Failed!");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssidAP, passAP);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    return;
  }
}
void initWeb() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/time", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(dayStamp).c_str());
  });
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String inputMessage;
    String inputParam;
    if (request->hasParam(PARAM_ON)) {
      inputMessage = request->getParam(PARAM_ON)->value();
      inputParam = PARAM_ON;
      onTime = inputMessage.toInt() * 1000;
      inputMessage = String(inputMessage.toInt() * 1000);
      writeFile(SPIFFS, "/ontime", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_OFF)) {
      inputMessage = request->getParam(PARAM_OFF)->value();
      inputParam = PARAM_OFF;
      offTime = inputMessage.toInt() * 1000;
      inputMessage = String(inputMessage.toInt() * 1000);
      writeFile(SPIFFS, "/offtime", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_HOUR_ON)) {
      inputMessage = request->getParam(PARAM_HOUR_ON)->value();
      inputParam = PARAM_HOUR_ON;
      hour_on = inputMessage.toInt();
      inputMessage = String(inputMessage.toInt());
      writeFile(SPIFFS, "/hour_on", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_MIN_ON)) {
      inputMessage = request->getParam(PARAM_MIN_ON)->value();
      inputParam = PARAM_MIN_ON;
      min_on = inputMessage.toInt();
      inputMessage = String(inputMessage.toInt());
      writeFile(SPIFFS, "/min_on", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_HOUR_OFF)) {
      inputMessage = request->getParam(PARAM_HOUR_OFF)->value();
      inputParam = PARAM_HOUR_OFF;
      hour_off = inputMessage.toInt();
      inputMessage = String(inputMessage.toInt());
      writeFile(SPIFFS, "/hour_off", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_MIN_OFF)) {
      inputMessage = request->getParam(PARAM_MIN_OFF)->value();
      inputParam = PARAM_MIN_OFF;
      min_off = inputMessage.toInt();
      inputMessage = String(inputMessage.toInt());
      writeFile(SPIFFS, "/min_off", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_SSID)) {
      inputMessage = request->getParam(PARAM_SSID)->value();
      inputParam = PARAM_SSID;
      writeFile(SPIFFS, "/ssid", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_PASSWORD)) {
      inputMessage = request->getParam(PARAM_PASSWORD)->value();
      inputParam = PARAM_PASSWORD;
      writeFile(SPIFFS, "/pass", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" + inputParam + ") with value: " + inputMessage + "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  AsyncElegantOTA.begin(&server);
  server.begin();
  Serial.println("HTTP сервер запущен!");
  Serial.println("..............................................");
}
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
void initVar() {
  File ontime = SPIFFS.open("/ontime", "r");
  if (!ontime || ontime.isDirectory()) {
    Serial.println("Ошибка чтения /ontime");
    return;
  }
  while (ontime.available()) {
    onContent += String((char)ontime.read());
  }
  ontime.close();
  onTime = onContent.toInt();
  File offtime = SPIFFS.open("/offtime", "r");
  if (!offtime || offtime.isDirectory()) {
    Serial.println("Ошибка чтения /offtime");
    return;
  }
  while (offtime.available()) {
    offContent += String((char)offtime.read());
  }
  offtime.close();
  offTime = offContent.toInt();
  File ssid_conf = SPIFFS.open("/ssid", "r");
  if (!ssid_conf || ssid_conf.isDirectory()) {
    Serial.println("Ошибка чтения /ssid");
    return;
  }
  while (ssid_conf.available()) {
    ssidContent += String((char)ssid_conf.read());
  }
  ssidContent.trim();
  int len = ssidContent.length() + 1;
  ssid[len];
  ssidContent.toCharArray(ssid, len);
  ssid_conf.close();
  File pass_conf = SPIFFS.open("/pass", "r");
  if (!pass_conf || pass_conf.isDirectory()) {
    Serial.println("Ошибка чтения /pass");
    return;
  }
  while (pass_conf.available()) {
    passContent += String((char)pass_conf.read());
  }
  passContent.trim();
  int len1 = passContent.length() + 1;
  password[len1];
  passContent.toCharArray(password, len1);
  pass_conf.close();
  File hour_ons = SPIFFS.open("/hour_on", "r");
  if (!hour_ons || hour_ons.isDirectory()) {
    Serial.println("Ошибка чтения /hour_on");
    return;
  }
  while (hour_ons.available()) {
    hour_on_Content += String((char)hour_ons.read());
  }
  hour_ons.close();
  hour_on = hour_on_Content.toInt();
  File min_ons = SPIFFS.open("/min_on", "r");
  if (!min_ons || min_ons.isDirectory()) {
    Serial.println("Ошибка чтения /min_on");
    return;
  }
  while (min_ons.available()) {
    min_on_Content += String((char)min_ons.read());
  }
  min_ons.close();
  min_on = min_on_Content.toInt();
  File hour_offs = SPIFFS.open("/hour_off", "r");
  if (!hour_offs || hour_offs.isDirectory()) {
    Serial.println("Ошибка чтения /ontime");
    return;
  }
  while (hour_offs.available()) {
    hour_off_Content += String((char)hour_offs.read());
  }
  hour_offs.close();
  hour_off = hour_off_Content.toInt();
  File min_offs = SPIFFS.open("/min_off", "r");
  if (!min_offs || min_offs.isDirectory()) {
    Serial.println("Ошибка чтения /min_off");
    return;
  }
  while (min_offs.available()) {
    min_off_Content += String((char)min_offs.read());
  }
  min_offs.close();
  min_off = min_off_Content.toInt();
}
void print_conf() {
  Serial.println("Считываем переменные с SPIFFS...\n..............................................");
  delay(1000);
  Serial.println("Конфигурация Wifi.\nИмя ssid: " + String(ssid) + "\nПароль: " + String(password));
  delay(1000);
  Serial.println("..............................................");
  Serial.println("Конфигурация циклического таймера:");
  Serial.println("Продолжительность работы (мс.): " + String(onTime) + "\n" + "Продолжительность простоя (мс.): " + String(offTime));
  delay(1000);
  Serial.println("..............................................");
  Serial.println("Конфигурациятаймера по времени:");
  Serial.println("Время включения: " + String(hour_on) + ":" + String(min_on) + "\nВремя выключения: " + String(hour_off) + ":" + String(min_off));
  Serial.println("..............................................");
  delay(1000);
  Serial.print("Локальный айпишник: ");
  Serial.println(WiFi.localIP());
  Serial.println("..............................................");
  getTime();
  dayTime = "Текущая дата:  " + String(dayStamp) + " \n";
  dayTime += "Текущее время:  " + String(timeStamp);
  Serial.println(dayTime);
  Serial.println("..............................................");
}
void getTime() {
  formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
}
void initTime() {
  timeClient.begin();
  timeClient.setTimeOffset(timeOffset);
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
}
void timer_loop() {
  digitalWrite(RELAY, RELAYstate);
  currentMillis = millis();
  if ((unsigned long)(currentMillis - previousMillis) >= interval) {
    if (RELAYstate) {
      interval = offTime;
      Serial.println();
      Serial.println("Выключаем на " + String(offTime) + "ms");
    } else {
      interval = onTime;
      Serial.println();
      Serial.println("Включаем на " + String(onTime) + "ms");
    }
    RELAYstate = !(RELAYstate);
    previousMillis = currentMillis;
  }
}
void timer_time_loop() {
  digitalWrite(RELAY, RELAYstate);
  if (millis() - timerMillis > 10000) {
    timerMillis = millis();
    while (!timeClient.update()) {
      timeClient.forceUpdate();
    }
    hour_now = timeClient.getHours();
    min_now = timeClient.getMinutes();
    if (hour_on == hour_off) {
      Serial.println("Час включения равен часу выключения");
      if (hour_now == hour_off) {
        Serial.println("Текущий час равен часу выключения");
        if (min_off > min_on || min_off < min_on) {
          if (min_now >= min_on && min_now < min_off) {
            RELAYstate = true;
            Serial.println("Relay state :" + String(RELAYstate));
          }
          else {
            RELAYstate = false;
            Serial.println("Relay state :" + String(RELAYstate));
          }
        }
      }
      else {
        RELAYstate = false;
        Serial.println("Relay state :" + String(RELAYstate));
      }
    }

    else if (hour_on > hour_off || hour_on < hour_off) {
      Serial.println("Час включения больше или меньше часа выключения");
      if (hour_now > hour_on && hour_now < hour_off) {
        Serial.println("текущий час больше часа старта и меньше часа выключения");
        RELAYstate = true;
        Serial.println("Relay state :" + String(RELAYstate));
      }
      else if (hour_now == hour_on) {
        Serial.println("Текущий час равен часу включения");
        if (min_now >= min_on) {
          RELAYstate = true;
          Serial.println("Relay state :" + String(RELAYstate));
        }
        else {
          RELAYstate = false;
          Serial.println("Relay state :" + String(RELAYstate));
        }
      }
      else if (hour_now == hour_off) {
        Serial.println("Текущий час равен часу выключения");
        if (min_now >= min_off) {
          RELAYstate = false;
          Serial.println("Relay state :" + String(RELAYstate));
        }
        else {
          RELAYstate = true;
          Serial.println("Relay state :" + String(RELAYstate));
        }
      }
      else {
        Serial.println("else");
        RELAYstate = false;
        Serial.println("Relay state :" + String(RELAYstate));
      }
    }
  }
}
void setup() {
  Serial.begin(115200);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  initSPIFFS();
  initVar();
  initWifi();
  initWeb();
  print_conf();
}
void loop() {
  AsyncElegantOTA.loop();
  //  timer_loop();
  timer_time_loop();
}
