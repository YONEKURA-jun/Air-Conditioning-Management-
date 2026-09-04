
AirCon Manager

Arduino クライアントと Windows ホスト間のシリアル通信による空調管理・監視システムです。

概要

ホスト PC から対象クライアントを選択し、温湿度情報の取得や機器制御を行います。

システム構成
HOST
C言語
Windows API
シリアル通信(9600bps)
CLIENT
Arduino Uno
DHT11 温湿度センサ
4桁7セグメントLED
74HC595 シフトレジスタ
実装済み機能
HOST
クライアント選択
COMポート接続
温湿度要求コマンド送信
電源ON/OFFコマンド送信
温湿度データ受信
ログ保存
ログ表示
CLIENT
温湿度測定
コマンド受信
電源状態管理
LED通知
7セグメントLED表示
シリアル応答
通信コマンド
値	内容1	温湿度取得
2	温度設定変更（実装中）
3	電源ON/OFF
ログフォーマット
ID,温度,湿度,年,月,日,時,分
0001,26.5,46.2,2026,9,4,14,35

開発メモ
解決済み不具合
Windows改行コード \r\n により COMポート名末尾へ \r が混入
CreateFileA() が ERROR_INVALID_NAME(123) を返却
strtok(address_data, ",\r\n") にて対応
7セグメントLED
74HC595 を利用した多重化表示
共通アノード構成
セグメントテーブルを反転して制御
今後の実装予定
温度設定変更機能
目標温度管理
ブザー通知
自動制御
ログ検索機能
エラー処理強化
開発環境
Visual Studio 2022
Arduino IDE
Windows
Arduino Uno R3
