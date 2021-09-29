#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#define RELAY 2 
boolean RELAYstate = true;
unsigned long previousMillis;
unsigned long currentMillis;
int onTime = 3000; //вводим время полива в ms
int offTime = 15000; // вводим время без полива в ms))
int interval = onTime;
const char* PARAM_ON = "input_ON";
const char* PARAM_OFF = "input_OFF";
const char* PARAM_SSID = "input_ssid";
const char* PARAM_PASSWORD = "input_password";
const char* ssid = "3a_aJIuaHc";          // Your WiFi SSID
const char* password = "**********";
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

AsyncWebServer server(80);
void initWifi(){
  const char* ssidAP = "3a_opgyc_hidden";
  const char* passAP = "***********";
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
    request->send_P(200, "text/html", index_html);
  });
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_ON)) {
      inputMessage = request->getParam(PARAM_ON)->value();
      inputParam = PARAM_ON;
      onTime = inputMessage.toInt()*1000*60;
      Serial.print("Время работы изменено:");
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_OFF)) {
      inputMessage = request->getParam(PARAM_OFF)->value();
      inputParam = PARAM_OFF;
      offTime = inputMessage.toInt()*1000*60;
      Serial.print("Время простоя изменено:");
    }
    else if (request->hasParam(PARAM_SSID)) {
      inputMessage = request->getParam(PARAM_SSID)->value();
      inputParam = PARAM_SSID;
      onTime = inputMessage.toInt();
      Serial.print("Время работы изменено:");
    }
    else if (request->hasParam(PARAM_PASSWORD)) {
      inputMessage = request->getParam(PARAM_PASSWORD)->value();
      inputParam = PARAM_PASSWORD;
      onTime = inputMessage.toInt();
      Serial.print("Время работы изменено:");
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
  pinMode(RELAY,OUTPUT);                      
  digitalWrite(RELAY, LOW);
  initWifi();
  initWeb();
}
void loop(){
  AsyncElegantOTA.loop();
  timer_loop();  
}
