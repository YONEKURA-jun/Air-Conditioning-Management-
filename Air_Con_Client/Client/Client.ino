#include <DHT.h>
#include <OneWire.h>

const int LED1 = 13;
const int LED2 = 12;
const int LED3 = 11;
const int LED4 = 10;
const int SW1 = A1;
const int SW2 = A2;
const int SW3 = A3;

const int POT = A0;
const int BUZZ = 3;

DHT dht(5, 11);

enum HOST_SIDE_COMMAND {
  RESPONSE_TEMP = 1,
  CHANGE_TEMP = 2,
  POWER_ON_OFF = 3,
};

bool g_status = false;

void power_flip();
void power_onSign();
void power_OffSign();
void power_react();

void to_receive_react();
void Action_select(int command);
void clean_Receive_Buff();

int get_temper();

void setup() {
  Serial.begin(9600);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);

  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
  digitalWrite(LED3, HIGH);
  digitalWrite(LED4, HIGH);

  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(SW3, INPUT_PULLUP);

  dht.begin();
}

void loop() {
  power_flip();
  to_receive_react();
}

void power_flip() {
  static bool loop_lock = false;

  if (digitalRead(SW2) == LOW && !loop_lock) {
    g_status = !g_status;
    loop_lock = true;
    power_react();
  }
  else if (digitalRead(SW2) == HIGH) {
    loop_lock = false;
  }
}

void power_onSign() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED1, LOW);
    delay(50);
    digitalWrite(LED1, HIGH);

    digitalWrite(LED2, LOW);
    delay(50);
    digitalWrite(LED2, HIGH);

    digitalWrite(LED3, LOW);
    delay(50);
    digitalWrite(LED3, HIGH);

    digitalWrite(LED4, LOW);
    delay(50);
    digitalWrite(LED4, HIGH);
  }

  digitalWrite(LED2, LOW);
}

void power_OffSign() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, LOW);

    delay(50);

    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, HIGH);

    delay(50);
  }
}

void to_receive_react() {
  int incomingChar;

  if (Serial.available() > 0) {

    incomingChar = Serial.read();

    if (g_status == false && incomingChar != 3) {
      Serial.println("PowerIsOff");
      Serial.read();
    }
    else if (g_status == true || incomingChar == 3) {
      Action_select(incomingChar);
    }
  }
}

void Action_select(int command) {

  switch (command) {

    case RESPONSE_TEMP:
      get_temper();
      break;

    case CHANGE_TEMP:
      break;

    case POWER_ON_OFF:
      g_status = !g_status;
      power_react();
      break;

    default:
      break;
  }
}

int get_temper() {

  float temper = dht.readTemperature();
  float humid  = dht.readHumidity();

  Serial.print("humid:");
  Serial.print(humid);
  Serial.print("-temper:");
  Serial.println(temper);
}

void clean_Receive_Buff() {
  while (Serial.available() > 0) {
    Serial.read();
  }
}

void power_react() {

  if (g_status == false) {
    power_onSign();
  }
  else if (g_status == true) {
    power_OffSign();
    clean_Receive_Buff();
  }
}