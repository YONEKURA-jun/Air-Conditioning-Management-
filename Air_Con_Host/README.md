# AirCon Manager

Arduino クライアントと Windows ホスト間のシリアル通信による空調管理・監視システムです。

Windows API を利用したデバイス監視機能により、USBシリアルデバイスの接続・切断をリアルタイムで検知し、自動的に管理対象へ登録します。

---

## 概要

本システムは中央管理用の Windows アプリケーション（HOST）と、温湿度測定・機器制御を行う Arduino クライアント（CLIENT）で構成されています。

HOST は接続中のクライアントを管理し、温湿度データ取得、設定温度変更、電源制御、通信監視などを行います。

また、起動時には登録済みクライアントの生存確認を実施し、応答の無い接続情報を自動的に除外します。

---

## システム構成

### HOST

- C言語
- Win32 API
- SetupAPI
- シリアル通信（9600bps）
- マルチスレッド構成
- デバイス監視機能
- COMポート自動取得
- PING/PONGによる死活監視

### CLIENT

- Arduino Uno R3
- DHT11 温湿度センサ
- UART通信
- 空調制御ロジック

---

## 実装済み機能

### HOST

#### 管理機能

- クライアント選択
- センサー情報取得
- 設定温度取得
- 設定温度変更
- 電源ON/OFF制御
- DTR制御によるクライアント再起動
- 通信状態確認

#### 通信機能

- COMポート接続
- シリアル送受信
- タイムアウト管理
- PING/PONGによる死活監視

#### ログ管理

- ログ表示
- ログ保存
- タイムスタンプ記録

#### デバイス管理

- USBデバイス挿抜監視
- COMポート接続・切断通知
- COMポート自動取得
- 接続機器名取得
- デバイス自動登録
- クライアント自動探索
- 無効クライアント自動除外

#### 並列処理

- デバイス監視専用スレッド
- CriticalSectionによる排他制御

### CLIENT

- 温湿度測定
- コマンド受信
- 電源状態管理
- 設定温度保持
- 設定温度変更
- シリアル応答
- PING応答

---

## 通信コマンド

HOST から CLIENT へは 2Byte 構成で送信を行います。

```text
Byte0 : コマンド
Byte1 : パラメータ
```

| 値 | 内容 |
|------|------|
| 0 | センサー情報取得 |
| 1 | 設定温度取得 |
| 2 | 設定温度変更 |
| 3 | 電源ON/OFF |
| 255 | PING/PONG |

### 温度変更シーケンス

```text
HOST
 ↓
設定温度取得
 ↓
CLIENT
 ↓
現在設定温度返信
 ↓
HOST
 ↓
新温度入力
 ↓
設定温度送信
 ↓
CLIENT
 ↓
設定値更新
```

### PING/PONGシーケンス

```text
HOST
 ↓
255(PING)
 ↓
CLIENT
 ↓
ping_pong,255
 ↓
HOST
 ↓
接続確認
```

---

## ログフォーマット

```text
ID,温度,湿度,年,月,日,時,分
```

例

```text
ROOM_A,26.5,46.2,2026,9,4,14,35
ROOM_B,25.8,48.7,2026,9,5,09,12
```

保存先

```text
Client_Log.txt
```

---

## 管理ファイル

### Client_address.txt

接続済みクライアント管理用ファイル

```text
デバイス名,COMポート
```

例

```text
Arduino Uno,COM5
Arduino Uno,COM7
```

---

### temp.txt

接続確認時に使用する一時ファイル

```text
Client_address.txt
再生成時に利用
```

---

## Windows API 実装

### シリアルポート監視

- RegisterDeviceNotification()
- WM_DEVICECHANGE
- CreateWindowEx()
- Message Only Window
- CreateThread()

### COMポート情報取得

- SetupDiGetClassDevs()
- SetupDiOpenDeviceInterfaceW()
- SetupDiGetDeviceInterfaceDetailW()
- SetupDiOpenDevRegKey()
- RegQueryValueExA()

### 動作概要

専用監視スレッドを生成し、Message Only Window を利用して Windows からのデバイス通知を受信します。

```text
USB接続
    ↓
WM_DEVICECHANGE
    ↓
WndProc()
    ↓
DEV_BROADCAST_DEVICEINTERFACE
    ↓
COMポート取得
    ↓
アドレスファイル登録
```

---

## シリアル通信設定

```text
BaudRate : 9600
DataBits : 8bit
StopBit  : 1bit
Parity   : None
```

### タイムアウト設定

```text
ReadIntervalTimeout       = 50ms
ReadTotalTimeoutConstant  = 1000ms
WriteTotalTimeoutConstant = 1000ms
```

---

## 開発メモ

### 解決済み不具合

#### COMポート接続失敗

Windows改行コード `\r\n` により COMポート文字列末尾へ `\r` が混入。

```c
CreateFileA()
```

が

```text
ERROR_INVALID_NAME (123)
```

を返却。

対策：

```c
strtok(address_data, ",\r\n");
```

により改行除去。

---

#### メインスレッド停止

デバイス監視機能実装時、

```c
GetMessage()
```

をメインスレッドで実行すると管理画面が停止。

原因：

```text
GetMessage()
       ↓
待機状態
       ↓
メイン処理停止
```

対策：

```c
CreateThread()
```

で監視専用スレッドを作成。

```text
監視スレッド
       ↓
Message Loop

メインスレッド
       ↓
管理画面処理
```

として並列実行。

---

#### アドレスファイル競合

監視スレッドとメインスレッドが同時に

```text
Client_address.txt
```

へアクセスする可能性があった。

問題：

```text
読込中
 +
書込中
```

↓

```text
ファイル破損
```

対策：

```c
CRITICAL_SECTION
```

を導入。

```c
EnterCriticalSection();
LeaveCriticalSection();
```

による排他制御を実装。

---

#### COMポート占有による通信失敗

調査結果：

```text
CreateFileA()
      ↓
ERROR_ACCESS_DENIED (5)
```

原因：

```text
Arduino IDE
シリアルモニタ起動中
      ↓
COMポート占有
```

対策：

```text
シリアルモニタ終了
      ↓
COMポート解放
      ↓
正常接続
```

---

#### PING/PONG通信

クライアント接続確認用として PING/PONG 通信を追加。

```text
HOST
  ↓
255(PING)
  ↓
CLIENT
  ↓
ping_pong,255
  ↓
接続確認
```

応答成功時

```c
temp->status = true;
```

として有効判定。

---

#### クライアント自動探索

起動時に登録済みクライアントへ接続確認を行い、応答の無いクライアントを自動除外。

```text
Client_address.txt
      ↓
PING送信
      ↓
応答確認
      ↓
有効クライアント抽出
      ↓
アドレスファイル再構築
```

---

#### COMポート自動取得

接続通知から取得したデバイスパスを基に COM ポート番号を取得。

```text
デバイスパス
      ↓
SetupDiOpenDeviceInterfaceW()
      ↓
SP_DEVINFO_DATA取得
      ↓
SetupDiOpenDevRegKey()
      ↓
PortName取得
      ↓
COM5
```

---

#### デバイス名自動取得

Friendly Name を取得し、管理情報として利用。

取得例

```text
Arduino Uno (COM5)
```

↓

```text
Arduino Uno
```

不要部分を除去。

```c
char* p = strstr(mech_name, " (COM");
if (p) *p = '\0';
```

---

#### DTR制御によるク*イアント再起動

Arduino の自*リセット機能を利用し、遠隔再起動*能を実装。

```c*EscapeCommFunction(client, SETDTR)*
Sleep(100);

EscapeCommFunction(c*ient, CLRDTR);
```

*``text
DTR ON
 ↓
DTR OFF
 ↓
Arduin* Reset
```

本機*は通信コマンドではなく、HOST 側による D*R ライン制御で実装している。

---

*### 通信タイムアウト調整

Arduino 起*直後の通信失敗*策としてタイムアウトを設定。

```c*ReadTotalTimeoutConstant  = 1000;
*riteTotal*imeoutConstant = 1000;
```

さらに

`*`c
Sleep(2000);
```

を挿入し起動完了を待機。
*---

*### デバイス通知機能

Windows デ*イス通知機能を*用し、接続・切断された COM デバイスをリア*タイムに検知。

```text*USB接続
      ↓
WM_DEVICECHANGE
    * ↓
WndProc()
      ↓
DEV_BROADCAST*DEVICEINTERFACE
      ↓
接続*ベント処理
```

*--

#### 無効クライアント自動除外

存在し*い COM 情報がアドレスファイル*残り続ける問題を解決。

```text*Client_address.txt
       ↓
全件確認
 *     ↓
PING送**       ↓
応答確認
       ↓
temp.txt生成
*      ↓
有効デバイスのみ抽出
       ↓*アド*ス*ァイル再生成
```

---

## ファイル構成

```*ext
HOST
├─ main.c
├─ Client_Log.t*t
├─ Client_address.txt
└─ temp.tx*
```

---

## 使用技術

### Windows

-*Win32 API
- SetupAPI
- シリアル通信
- デバ*ス通知
- Message Loop
- Message Only *indow
- マルチスレッド
- Callback関数
- Cri*ical Section

### Arduino

- UART通*
- DHT11
- 状態管理
- シ*アルプロトコル設計

---

## 今後の実装予定

- GUI*
-*自動温度監視
- 定期ポーリング
-*ログ検索機能
- CSV出力
- 設**ァイル対応
- 自*復旧機*
- エラー処理強化
- 複数クライアント同時監視

---

##*開発環境

### HOST

- Visual Studio 20*2
- Windows 11

### CLIENT

- Ardu*no IDE
- Arduino Uno R3

---

## 学習内容

本開発を通じて以下を習得した。

- Windows API*を用いたデバイス監視
- Message Loop の仕組み
- M*ssage Only Window の実装
- Callback関数*よるイベント処理
- マルチスレッドプログラミング
- Critic*lSectionによる排他制御
- シリアル通信プロトコル設計
- *rduino と PC 間通信
- PING/PONGによる死活監視*- DTR制御によるArduinoリセット
- ログ管理機能の実装
* SetupAPIを利用したデバイス情報取得
- レジストリ経由のC*Mポート取得
- COMポート自動検出
- デバイス通知情報とCOM*ート情報の関連付け
- USBデバイス自動登録
- イベント駆動型ア*リケーション設計
- Windowsエラーコードを利用した障害切り分*
- COMポート占有時の挙動確認
-*組み込み機器と Windows アプリケーション連携
````*