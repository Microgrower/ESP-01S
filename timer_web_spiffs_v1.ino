#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <Hash.h>
#include <FS.h>
#define RELAY 2 
boolean RELAYstate = true;
unsigned long previousMillis, currentMillis;
int onTime, offTime;
int interval = onTime;
String onContent, offContent, ssidContent, passContent;
const char* PARAM_ON = "input_ON";
const char* PARAM_OFF = "input_OFF";
const char* PARAM_SSID = "input_ssid";
const char* PARAM_PASSWORD = "input_password";
char ssid[24];         
char password[32];
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html lang="ru"><head><meta charset="UTF-8">
  <title>ESP Timer Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h1>Настройки автоподлива</h1>
  <h2>Настройки WiFi</h2>
    <form action="/get">
    Введите точку доступа: <input type="text" name="input_ssid">
    <input type="submit" value="Тыкай!">
  </form><br>
  <form action="/get">
    Введите пароль:<input type="text" name="input_password">
    <input type="submit" value="Тыкай!">
  </form><br>
  <h2>Настройки таймера</h2>
  <form action="/get">
    Время работы(в минутах): <input type="number" name="input_ON">
    <input type="submit" value="Тыкай!">
  </form><br>
  <form action="/get">
    Время простоя (в минутах): <input type="number" name="input_OFF">
    <input type="submit" value="Тыкай!">
  </form><br>
</body></html>)rawliteral";
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
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
  file.close();
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
  file.close();
}
String processor(const String& var){
  //Serial.println(var);
  if(var == "input_ssid"){
    return readFile(SPIFFS, "/ssid");
  }
  else if(var == "input_password"){
    return readFile(SPIFFS, "/pass");
  }
  else if(var == "input_ON"){
    return readFile(SPIFFS, "/ontime");
  }
  else if(var == "input_OFF"){
    return readFile(SPIFFS, "/offtime");
  }
  return String();
}
AsyncWebServer server(80);
void initWifi(){
  const char* ssidAP = "Умный_Дом";
  const char* passAP = "517500FeFe";
  IPAddress local_IP(192, 168, 1, 150);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 0, 0);
  IPAddress primaryDNS(8, 8, 8, 8);
  IPAddress secondaryDNS(8, 8, 4, 4);
  WiFi.mode(WIFI_STA); 
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("Не могу подключиться к вафле. Запускаю SoftAP\n");
    delay(1000);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssidAP, passAP, 1, 1);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    return;
  }
  Serial.println();
  delay(1000);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}
void initWeb(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;

    if (request->hasParam(PARAM_ON)) {
      inputMessage = request->getParam(PARAM_ON)->value();
      inputParam = PARAM_ON;
      onTime = inputMessage.toInt()*1000;
      inputMessage = String(inputMessage.toInt()*1000);
      Serial.print("Время работы изменено:");
      writeFile(SPIFFS, "/ontime", inputMessage.c_str());
      }
    else if (request->hasParam(PARAM_OFF)) {
      inputMessage = request->getParam(PARAM_OFF)->value();
      inputParam = PARAM_OFF;
      offTime = inputMessage.toInt()*1000;
      inputMessage = String(inputMessage.toInt()*1000);
      Serial.print("Время простоя изменено:");
      writeFile(SPIFFS, "/offtime", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_SSID)) {
      inputMessage = request->getParam(PARAM_SSID)->value();
      inputParam = PARAM_SSID;
      //ssid = inputMessage.c_str();
      Serial.print("Точка доступа изменена");
      writeFile(SPIFFS, "/ssid", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_PASSWORD)) {
      inputMessage = request->getParam(PARAM_PASSWORD)->value();
      inputParam = PARAM_PASSWORD;
      //password = inputMessage.c_str();
      Serial.print("Пароль изменен:");
      writeFile(SPIFFS, "/pass", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" + inputParam + ") with value: " + inputMessage +"<br><a href=\"/\">Return to Home Page</a>");   
  });
  server.onNotFound(notFound);  
  AsyncElegantOTA.begin(&server);
  server.begin();
  Serial.println("HTTP сервер запущен!");
}
void initVar(){
  File ontime = SPIFFS.open("/ontime", "r");
  if(!ontime || ontime.isDirectory()){
    Serial.println("Ошибка чтения /ontime");
    return;
  }
  while(ontime.available()){
    onContent+=String((char)ontime.read());
  }
  ontime.close();
  onTime = onContent.toInt();  
  File offtime = SPIFFS.open("/offtime", "r");
  if(!offtime || offtime.isDirectory()){
    Serial.println("Ошибка чтения /offtime");
    return;
  }
  while(offtime.available()){
    offContent+=String((char)offtime.read());
  }
  offtime.close();
  offTime = offContent.toInt();    
  File ssid_conf = SPIFFS.open("/ssid", "r");
  if(!ssid_conf || ssid_conf.isDirectory()){
    Serial.println("Ошибка чтения /ssid");
    return;
  }
  while(ssid_conf.available()){
    ssidContent+=String((char)ssid_conf.read());
  }
  ssidContent.trim();
  int len = ssidContent.length()+1;
  ssid[len];
  ssidContent.toCharArray(ssid, len);  
  ssid_conf.close();  
  File pass_conf = SPIFFS.open("/pass", "r");
  if(!pass_conf || pass_conf.isDirectory()){
    Serial.println("Ошибка чтения /pass");
    return;
  }
  while(pass_conf.available()){
    passContent+=String((char)pass_conf.read());
  }
  passContent.trim();
  int len1 = passContent.length()+1;
  password[len];
  passContent.toCharArray(password, len);
  pass_conf.close();       
}
void initSPIFFS(){
  if(!SPIFFS.begin()){
      Serial.println("Ошибка монтирования SPIFFS");
      return;
    }
  else{
    Serial.println("SPIFFS примонтирована");   
  } 
}
void timer_loop(){
  digitalWrite(RELAY, RELAYstate);
  currentMillis = millis();
  if ((unsigned long)(currentMillis - previousMillis) >= interval){
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
void setup(){      
  Serial.begin(115200);
  Serial.println();
  delay(1000);
  initSPIFFS();
  pinMode(RELAY,OUTPUT);                      
  digitalWrite(RELAY, LOW);
  initVar();
  delay(3000);
  Serial.print("ssid: ");
  Serial.println(ssid);
  Serial.print("password: ");
  Serial.println(password);
  Serial.print("Время работы: ");
  Serial.println(onTime);
  Serial.print("Время простоя: ");
  Serial.println(offTime); 
  initWifi();
  initWeb();  
}
void loop(){
  AsyncElegantOTA.loop();
  timer_loop();  
}
