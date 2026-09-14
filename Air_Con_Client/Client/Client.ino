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

const int SDI = 8;
const int CLK = 7;
const int LCH = 4;

DHT dht(5, 11);

//７セグLedを点灯させるにあたっての値
const uint8_t digi[] = {
  0xEF,  // 0セグ　一番右
  0xDF,  // 1セグ
  0xBF,  // 2セグ
  0x7F,  // 3セグ
  0xff   // 消灯コマンド
};

const uint8_t nump[] = {
  0xFC,  // 0
  0x60,  // 1
  0xDA,  // 2
  0xF2,  // 3
  0x66,  // 4
  0xB6,  // 5
  0xBE,  // 6
  0xE4,  // 7
  0xFE,  // 8
  0xF6,  // 9
  0xFF   //消灯コマンド　10
};

int g_temper[4] = { 0, 0, 6, 2 };

bool g_status = false;
bool g_ready = false;

enum HOST_SIDE_COMMAND {
  RESPONSE_TEMP,
  CHANGE_TEMP,
  POWER_ON_OFF,
  PING_PONG = 255,
};


void power_flip();     //電源スイッチ
void power_onSign();   //電源ON時の挙動
void power_OffSign();  //off時の挙動
void power_react();    //電源の入り切りの際の挙動を決める

void to_receive_react();          //受信時の挙動を決める
void action_select(int command);  //受信時の挙動を決めている
void clean_Receive_Buff();        //受信バッファのデータを全消去する

int get_temper();  //センサーから温度と湿度を取得する

void Seg_choice(uint8_t dig, uint8_t num);  //引数digに対応するセグメントLedに、numの値を送信する。
void temperMonitor_OnOff(bool status);      //セグメントLed点灯
void SetTemp_send();                        //現在の設定温度をホストの呼びかけに返す
void change_SetTemp(int SetTemp);           //ホストから受け取った値に設定温度を変更する。

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

  pinMode(SDI, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(LCH, OUTPUT);

  dht.begin();
}

void loop() {
  power_flip();
  to_receive_react();
  temperMonitor_OnOff(g_status);
   get_pushSw();
}

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

void power_react() {

  if (g_status == true) {
    power_onSign();

  } else if (g_status == false) {
    power_OffSign();
    clean_Receive_Buff();
  }
}

void to_receive_react() {

  int incomingChar;
  int host_command = 0;

  if (Serial.available() > 0) {

    if (g_ready == true) {
      host_command = Serial.read();
      change_SetTemp(host_command);
      g_ready = false;
      return;
    }

    incomingChar = Serial.read();
    if (g_status == false && incomingChar != POWER_ON_OFF && incomingChar != PING_PONG) {
      Serial.print("PowerIsOff,");
      Serial.println(g_status);
      Serial.read();
    }

    else if (g_status == true || incomingChar == POWER_ON_OFF || incomingChar == PING_PONG) {
      action_select(incomingChar);
    }
  }
}

void action_select(int command) {
  switch (command) {

    case RESPONSE_TEMP:
      get_temper();
      break;

    case CHANGE_TEMP:
      SetTemp_send();
      break;

    case POWER_ON_OFF:
      g_status = !g_status;
      power_react();
      Serial.print("ok,");
      Serial.println(g_status);
      break;

    case PING_PONG:
      Serial.print("ping_pong,");
      Serial.println("255");
      break;


    default:
      break;
  }
}

int get_temper() {

  float temper = dht.readTemperature();
  float humid = dht.readHumidity();

  Serial.print("humid:");
  Serial.print(humid);
  Serial.print("-temper:");
  Serial.print(temper);
  Serial.print(",");
  Serial.println(g_status);
}

void clean_Receive_Buff() {
  while (Serial.available() > 0) {
    Serial.read();
  }
}

void Seg_choice(uint8_t dig, uint8_t num) {
  digitalWrite(LCH, LOW);
  shiftOut(SDI, CLK, LSBFIRST, nump[num]);
  shiftOut(SDI, CLK, LSBFIRST, digi[dig]);
  digitalWrite(LCH, HIGH);
}

void get_pushSw() {
  static bool loop_lock1 = false;
  static bool loop_lock2 = false;

  int set_temp = g_temper[3] * 10 + g_temper[2];

  if (digitalRead(SW1) == LOW && !loop_lock1) {
    loop_lock1 = true;

    if (set_temp < 99) {
      set_temp++;
    }
  }
  else if (digitalRead(SW1) == HIGH) {
    loop_lock1 = false;
  }

  if (digitalRead(SW3) == LOW && !loop_lock2) {
    loop_lock2 = true;

    if (set_temp > 0) {
      set_temp--;
    }
  }
  else if (digitalRead(SW3) == HIGH) {
    loop_lock2 = false;
  }

  g_temper[2] = set_temp % 10;
  g_temper[3] = set_temp / 10;
}

void temperMonitor_OnOff(bool status) {
  static int count = 0;

  if (status == true) {
    Seg_choice(count, g_temper[count]);
    count++;
  } else if (status == false) {
    Seg_choice(4, 10);
  }
  if (count >= 4) {
    count = 0;
  }
}

void SetTemp_send() {
  float set_temper = (g_temper[3] * 1000 + g_temper[2] * 100 + g_temper[1] * 10 + g_temper[0] / 100.0f);
  Serial.print(set_temper);
  Serial.print(",");
  Serial.println(g_status);
  g_ready = true;
}

void change_SetTemp(int settemp) {
  g_temper[0] = 0;
  g_temper[1] = 0;
  g_temper[2] = settemp % 10;
  g_temper[3] = (settemp / 10) % 10;
}
