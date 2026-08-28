#include <OneWire.h>
#include <DallasTemperature.h>

const int LED_L  = 13; // 本体 L LED
const int LED_TX = 1;  // 本体 TX LED
const int LED_RX = 0;  // 本体 RX LED

void setup() {
  Serial.begin(9600);
  
  pinMode(LED_L, OUTPUT);
  pinMode(LED_TX, OUTPUT);
  pinMode(LED_RX, OUTPUT);
  
  digitalWrite(LED_L, LOW);
  digitalWrite(LED_TX, HIGH); // TX/RXはLOWで点灯するためHIGHで消灯
  digitalWrite(LED_RX, HIGH);
}

void loop() {
  if (Serial.available() > 0) {
    char incomingChar = Serial.read();
    
    if (incomingChar == 'y') {
      // 1つ目 (L)
      digitalWrite(LED_L, HIGH);
      delay(2000);
      digitalWrite(LED_L, LOW);
    }
  }
}