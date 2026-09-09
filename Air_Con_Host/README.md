AirCon Manager
Arduino クライアントと Windows ホスト間のシリアル通信による空調管理・監視システムです。
概要
ホスト PC から対象クライアントを選択し、温湿度情報の取得や機器制御を行います。
また、Windows API を利用したシリアルポート監視機能により、Arduino の接続・切断をリアルタイムで検知できます。
システム構成
HOST
C言語
Windows API
Serial Communication (9600bps)
マルチスレッド構成
デバイス監視機能
CLIENT
Arduino Uno R3
DHT11 温湿度センサ
4桁7セグメントLED
74HC595 シフトレジスタ
実装済み機能
HOST
クライアント選択
COMポート接続
温湿度取得コマンド送信
温度設定変更
電源ON/OFFコマンド送信
温湿度データ受信
ログ保存
ログ表示
USBシリアルデバイス監視
COMポート接続・切断通知
CLIENT
温湿度測定
コマンド受信
電源状態管理
LED通知
7セグメントLED表示
シリアル応答
設定温度管理
設定温度変更受信
通信コマンド
値	内容0	温湿度取得
1	温度設定変更
2	電源ON/OFF
温度変更シーケンス
HOST → CHANGE_TEMP送信
CLIENT → 現在設定温度返信
HOST → 新設定温度送信
CLIENT → 設定温度更新
ログフォーマット
Plain Text
1
ID,温度,湿度,年,月,日,時,分
2
 
3
0001,26.5,46.2,2026,9,4,14,35
4
0002,25.8,48.7,2026,9,5,09,12
その他の行を表示する
Windows API 実装
シリアルポート監視
RegisterDeviceNotification()
WM_DEVICECHANGE
CreateWindowEx()
Message Only Window
CreateThread()
動作概要
専用監視スレッドを生成し、Message Only Window を利用して Windows からのデバイス通知を受信します。
Plain Text
1
USB接続
2
↓
3
WM_DEVICECHANGE
4
↓
5
WndProc()
6
↓
7
接続イベント処理
その他の行を表示する
開発メモ
解決済み不具合
COMポート接続失敗
Windows改行コード \r\n により COMポート文字列末尾へ \r が混入。
C
1
CreateFileA()
その他の行を表示する
が
Plain Text
1
ERROR_INVALID_NAME (123)
その他の行を表示する
を返却。
対策：
C
1
strtok(address_data, ",\r\n");
その他の行を表示する
により改行除去。
温度変更通信
変更コマンド送信後に追加データ受信が必要となったため、クライアント側へ受信待機フラグを実装。
Plain Text
1
CHANGE_TEMP
2
↓
3
現在設定温度返信
4
↓
5
受信待機状態
6
↓
7
新設定温度受信
8
↓
9
設定値更新
その他の行を表示する
デバイス監視
メッセージループ実装時、GetMessage() によりメインスレッドがブロック。
対策：
C
1
CreateThread()
その他の行を表示する
で監視専用スレッドを作成し、コンソール処理と並列実行を実現。
使用技術
Windows
Win32 API
シリアル通信
デバイス通知
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
クライアント自動探索
ログ検索機能
設定ファイル化
エラー処理強化
開発環境
Visual Studio 2022
Arduino IDE
Windows
Arduino Uno R3
学習内容
本開発を通じて以下を習得した。
Windows API を用いたデバイス監視
Message Loop の仕組み
Callback 関数によるイベント処理
マルチスレッドプログラミング
シリアル通信プロトコル設計
Arduino と PC 間通信
組み込み機器と Windows アプリケーション連携
ログ管理機能の実装
状態管理を用いた通信制御