#define RELAY 4 //пин, к которому подключена релюшка
boolean RELAYstate = true;
unsigned long previousMillis;
unsigned long currentMillis;
const unsigned int onTime = 3000; //вводим время полива в ms
const unsigned int offTime = 15000; // вводим время без полива в ms))
int interval = onTime;
void setup(){      
  Serial.begin(115200); 
  pinMode(RELAY,OUTPUT);                      
  digitalWrite(RELAY, LOW); 
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
