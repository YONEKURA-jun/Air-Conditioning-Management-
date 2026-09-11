# AirCon Manager

Arduino クライアントと Windows ホスト間のシリアル通信による空調管理・監視システムです。

---

## 概要

ホストPCから対象クライアントを選択し、温湿度情報の取得や機器制御を行います。

また、Windows API を利用したシリアルポート監視機能により、Arduino の接続・切断をリアルタイムで検知できます。

---

## システム構成

### HOST

- C言語
- Windows API
- シリアル通信（9600bps）
- マルチスレッド構成
- デバイス監視機能

### CLIENT

- Arduino Uno R3
- DHT11 温湿度センサ
- 4桁7セグメントLED
- 74HC595 シフトレジスタ

---

## 実装済み機能

### HOST

- クライアント選択
- COMポート接続
- 温湿度取得コマンド送信
- 温度設定変更
- 電源ON/OFFコマンド送信
- 温湿度データ受信
- ログ保存
- ログ表示
- USBシリアルデバイス監視
- COMポート接続・切断通知
- クライアント自動探索
- PING/PONG死活監視
- 接続デバイスのCOMポート自動取得

### CLIENT

- 温湿度測定
- コマンド受信
- 電源状態管理
- LED通知
- 7セグメントLED表示
- シリアル応答
- 設定温度管理
- 設定温度変更受信
- PING応答

---

## 通信コマンド

| 値 | 内容 |
|----|------|
| 0 | 温湿度取得 |
| 1 | 温度設定変更 |
| 2 | 電源ON/OFF |
| 255 | PING/PONG |

### 温度変更シーケンス

1. HOST → CHANGE_TEMP送信
2. CLIENT → 現在設定温度返信
3. HOST → 新設定温度送信
4. CLIENT → 設定温度更新

---

## ログフォーマット

```text
ID,温度,湿度,年,月,日,時,分

0001,26.5,46.2,2026,9,4,14,35
0002,25.8,48.7,2026,9,5,09,12

Windows API 実装
シリアルポート監視
RegisterDeviceNotification()
WM_DEVICECHANGE
CreateWindowEx()
Message Only Window
CreateThread()
動作概要

専用監視スレッドを生成し、Message Only Window を利用して Windows からのデバイス通知を受信します。

USB接続
    ↓
WM_DEVICECHANGE
    ↓
WndProc()
    ↓
接続イベント処理

開発メモ
解決済み不具合
COMポート接続失敗

Windows改行コード \r\n により COMポート文字列末尾へ \r が混入。

CreateFileA()


が

ERROR_INVALID_NAME (123)


を返却。

対策：

strtok(address_data, ",\r\n");


により改行除去。

温度変更通信

変更コマンド送信後に追加データ受信が必要となったため、クライアント側へ受信待機フラグを実装。

CHANGE_TEMP
      ↓
現在設定温度返信
      ↓
受信待機状態
      ↓
新設定温度受信
      ↓
設定値更新

デバイス監視

メッセージループ実装時、GetMessage() によりメインスレッドがブロック。

対策：

CreateThread()


で監視専用スレッドを作成し、コンソール処理と並列実行を実現。

クライアント自動探索

起動時に登録済みクライアントへ接続確認を行い、応答の無いクライアントをアドレス一覧から自動除外する機能を実装。

Client_address.txt
      ↓
PING送信
      ↓
応答確認
      ↓
有効クライアント抽出
      ↓
アドレスファイル再構築

PING/PONG通信

クライアント接続確認用として PING/PONG 通信を追加。

HOST
  ↓
255(PING)
  ↓
CLIENT
  ↓
ping_pong,255
  ↓
接続確認

COMポート占有による通信失敗

PING/PONG実装時に通信不能が発生。

調査結果：

CreateFileA()
      ↓
ERROR_ACCESS_DENIED (5)
      ↓
INVALID_HANDLE_VALUE
      ↓
WriteFile()
      ↓
ERROR_INVALID_HANDLE (6)


原因：

Arduino IDE
シリアルモニタ起動中
      ↓
COMポート占有
      ↓
CreateFileA失敗


対策：

シリアルモニタ終了
      ↓
COMポート解放
      ↓
正常接続

通信プロトコル不整合

PING/PONG実装中に HOST 側受信解析が失敗。

調査結果：

CLIENT
      ↓
0
      ↓
HOST
      ↓
受信解析失敗


原因：

PowerIsOff,
出力処理の欠落


対策：

PowerIsOff,0
      ↓
HOST
      ↓
sscanf("%[^,],%d")
      ↓
正常解析

デバイス情報取得

Windows デバイス通知を利用し、接続・切断された COM デバイスをリアルタイムに検出する機能を実装。

WM_DEVICECHANGE
      ↓
lParam
      ↓
DEV_BROADCAST_HDR
      ↓
DEV_BROADCAST_DEVICEINTERFACE
      ↓
デバイスパス取得

COMポート自動取得

通知で受信したデバイスパスから、対象デバイスへ割り当てられた COM ポートを取得する機能を実装。

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

使用技術
Windows
Win32 API
シリアル通信
デバイス通知
SetupAPI
イベント駆動プログラミング
マルチスレッド
Arduino
DHT11
UART通信
74HC595
7セグメントLED制御
状態遷移制御
今後の実装予定
目標温度管理機能
ブザー警告
自動温度制御
ポート再接続時の自動復旧
ログ検索機能
設定ファイル化
エラー処理強化
開発環境
Visual Studio 2022
Arduino IDE
Windows 11
Arduino Uno R3
学習内容

本開発を通じて以下を習得した。

Windows API を用いたデバイス監視
Message Loop の仕組み
Callback関数によるイベント処理
マルチスレッドプログラミング
シリアル通信プロトコル設計
Arduino と PC 間通信
組み込み機器と Windows アプリケーション連携
ログ管理機能の実装
状態管理を用いた通信制御
PING/PONGによる死活監視
Windowsエラーコードを利用した障害切り分け
COMポート占有時の挙動確認
SetupAPIを利用したデバイス情報取得
デバイス通知情報とCOMポート情報の関連付け