#include <OneWire.h>

const int LED_L  = 13; // 本体 L LED
bool g_temp = true;

void setup() {
  Serial.begin(9600); 
  pinMode(LED_L, OUTPUT);
  digitalWrite(LED_L, LOW);
}

void loop() {
  if (Serial.available() > 0) {
    char incomingChar = Serial.read();
    
    if (incomingChar == 'y') {
      digitalWrite(LED_L, HIGH);
      delay(500);
      digitalWrite(LED_L, LOW);
      g_temp = false;
    }

    if(g_temp == false){
      Serial.println("nice_work"); 
    }
  }
  g_temp = true;
}