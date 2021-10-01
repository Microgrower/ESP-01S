#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <Hash.h>
#include <FS.h>
#define RELAY 2 
#define EEPROM_SIZE 512
boolean RELAYstate = true;
unsigned long previousMillis;
unsigned long currentMillis;
int onTime; //вводим время полива в ms
int offTime; // вводим время без полива в ms))
int interval = onTime;
const char* PARAM_ON = "input_ON";
const char* PARAM_OFF = "input_OFF";
const char* PARAM_SSID = "input_ssid";
const char* PARAM_PASSWORD = "input_password";
const char* ssid = "********";          // Your WiFi SSID
const char* password = "********";
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Timer Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    Time ON work(minute): <input type="number" name="input_ON">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    Time OFF work(minute): <input type="number" name="input_OFF">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    WiFi ssid: <input type="text" name="input_ssid">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    WiFi password: <input type="text" name="input_password">
    <input type="submit" value="Submit">
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
  const char* ssidAP = "3a_opgy_hidden";
  const char* passAP = "517500FeFe";
  IPAddress local_IP(192, 168, 1, 150);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 0, 0);
  IPAddress primaryDNS(8, 8, 8, 8);
  IPAddress secondaryDNS(8, 8, 4, 4);
  WiFi.mode(WIFI_AP_STA); 
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("Не могу подключиться к вафле. Запускаю SoftAP\n");
    delay(1000);
    WiFi.softAP(ssidAP, passAP, 1, 1); //устанавливаем имя точки, пароль, канал и скрытый режим вещани
    return;
  }
  Serial.println();
  delay(1000);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
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
      onTime = inputMessage.toInt()*1000*60;
      inputMessage = String(inputMessage.toInt()*1000*60);
      Serial.print("Время работы изменено:");
      writeFile(SPIFFS, "/ontime", inputMessage.c_str());
      String yourInputString = readFile(SPIFFS, "/ontime");
      }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_OFF)) {
      inputMessage = request->getParam(PARAM_OFF)->value();
      inputParam = PARAM_OFF;
      offTime = inputMessage.toInt()*1000*60;
      inputMessage = String(inputMessage.toInt()*1000*60);
      Serial.print("Время простоя изменено:");
      writeFile(SPIFFS, "/offtime", inputMessage.c_str());
      String yourInputString = readFile(SPIFFS, "/offtime");
    }
    else if (request->hasParam(PARAM_SSID)) {
      inputMessage = request->getParam(PARAM_SSID)->value();
      inputParam = PARAM_SSID;
      ssid = inputMessage.c_str();
      Serial.print("Точка доступа изменена");
      writeFile(SPIFFS, "/ssid", inputMessage.c_str());
      String yourInputString = readFile(SPIFFS, "/ssid");
    }
    else if (request->hasParam(PARAM_PASSWORD)) {
      inputMessage = request->getParam(PARAM_PASSWORD)->value();
      inputParam = PARAM_PASSWORD;
      password = inputMessage.c_str();
      Serial.print("Пароль изменен:");
      writeFile(SPIFFS, "/pass", inputMessage.c_str());
      String yourInputString = readFile(SPIFFS, "/pass");
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
    Serial.println("- empty file or failed to open file");
    return;
  }
  Serial.println("- read from file:");
  String onContent;
  while(ontime.available()){
    onContent+=String((char)ontime.read());
  }
  ontime.close();
  onTime = onContent.toInt();  
  Serial.println(onContent);
  File offtime = SPIFFS.open("/offtime", "r");
  if(!offtime || offtime.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return;
  }
  Serial.println("- read from file:");
  String offContent;
  while(offtime.available()){
    offContent+=String((char)offtime.read());
  }
  offtime.close();
  offTime = offContent.toInt();  
  Serial.println(offContent);    
}
void initSPIFFS(){
  if(!SPIFFS.begin()){
      Serial.println("Ошибка монтирования SPIFFS");
      return;
    }
  else{
    Serial.println("SPIFFS примонтирована");
    delay(300);
    readFile(SPIFFS, "/ontime");
    delay(300);
    readFile(SPIFFS, "/offtime");
    delay(300);
    readFile(SPIFFS, "/ssid");
    delay(300);
    readFile(SPIFFS, "/pass");    
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
  initWifi();
  initWeb();  
}
void loop(){
  AsyncElegantOTA.loop();
  timer_loop();  
}
