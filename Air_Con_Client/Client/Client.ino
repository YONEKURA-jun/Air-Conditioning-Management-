//Client

#include <DHT.h>
#include <DHT_U.h>
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


enum HOST_SIDE_COMMAND {
  RESPONSE_TEMP =1,
  CHANGE_TEMP   =2,
  POWER_ON_OFF  =3,
};



bool g_status = true;



void power_flip();    //電源スイッチ
void power_onSign();   //スイッチONの可視化
void power_OffSign();  //スイッチOFFの可視化
void power_react();    //電源スイッチ押下時のリアクション。


void to_receive_react();           //HOSTからの呼びかけがあった際のリアクション。
void Action_select(int command);  //HOST側から与えられた命令に応える。
void clean_Receive_Buff();//受信バッファ内の情報を全消去する。

int get_temper();  //センサから温度を取得する。


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
}

void loop() {
  power_flip();
  to_receive_react();
}


//以下関数定義

void power_flip() {
  static bool loop_lock = false;
  if (digitalRead(SW2) == LOW && !loop_lock) {
    g_status = !g_status;
    loop_lock = true;
    power_react();
  } else if (digitalRead(SW2) == HIGH) {
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
    if (g_status == true) {  //電源Off
      Serial.println("PowerIsOff");
      Serial.read();
    } 
    else if (g_status == false) {  //電源ON
      incomingChar = Serial.read();
      Action_select(incomingChar);
    }
  }
}


void Action_select(int command) {
  int temp = 0;
  
  switch (command) {
    case RESPONSE_TEMP :
      temp = get_temper();
      Serial.println(temp);
      break;

    case CHANGE_TEMP :
      break;

    case POWER_ON_OFF:
      break;

    default:
      break;
  }
}

int get_temper() {
  int temp;
  temp = analogRead(POT);
  return (temp);
}

void clean_Receive_Buff(){
 while (Serial.available() > 0) {Serial.read();}
}

void power_react() {
  if (g_status == false) {
    power_onSign();
  } else if (g_status == true) {
    power_OffSign();
    clean_Receive_Buff();
  }
}
