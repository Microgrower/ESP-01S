#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#define RELAY 4 
boolean RELAYstate = true;
unsigned long previousMillis;
unsigned long currentMillis;
const unsigned int onTime = 3000; //вводим время полива в ms
const unsigned int offTime = 15000; // вводим время без полива в ms))
int interval = onTime;
AsyncWebServer server(80);
void WiFis(){
  const char* ssid = "3a_aJIuaHc";          // Your WiFi SSID
  const char* password = "**********";
  const char* ssidAP = "3a_opgy";
  const char* passAP = "************";
  IPAddress local_IP(192, 168, 1, 149);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 0, 0);
  IPAddress primaryDNS(8, 8, 8, 8);
  IPAddress secondaryDNS(8, 8, 4, 4);
  IPAddress ap_IP(192, 168, 0, 100);
  IPAddress ap_gateway(192, 168, 0, 100);
  IPAddress ap_subnet(255, 255, 0, 0);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssidAP, passAP);
  WiFi.softAPConfig(ap_IP, ap_gateway, ap_subnet);
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
}
void initWeb(){
  AsyncElegantOTA.begin(&server);
  server.begin();
  Serial.println("HTTP сервер запущен!");  
}
void setup(){      
  Serial.begin(115200); 
  pinMode(RELAY,OUTPUT);                      
  digitalWrite(RELAY, LOW); 
  WiFis();
  initWeb();
}
void loop(){
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
