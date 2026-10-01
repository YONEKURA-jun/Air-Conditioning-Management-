//HOST
#define _CRT_SECURE_NO_WARNINGS


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#include <Windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <dbt.h>
#pragma comment(lib, "setupapi.lib")


#define INPUT_CHECK -1
#define CLIENT_NAME 21
#define ALL_CLIENTS 15
#define LOG_SIZE 29
#define ID_SIZE_DATA 32
#define PASS_SIZE 32
#define ADDRESS_DATA 32
#define CHECK_DATA 8
#define CONNECT_TEST 255
#define MAX_SIZE 256
#define UNUSED 0


enum SCREENS_USER_CHOICE {
	SHOW_LOG,
	SEND_ORDER,
};

enum ERROR_REACTION {
	FILE_SYS,
	COMM_SYS,
	INPUT_SYS,
	STRUCT_SYS,
	WINDOWS_SYS,
	PINGPONG_SYS,
	CONNECT_SYS,
};


enum CLIENT_COMMAND {
	GET_SENSOR_STATUS,
	GET_TEMP,
	SET_TEMP,
	PING_PONG = 255,
};


enum USER_REQUEST {
	SHOW_SENSOR_STATUS,
	SHOW_TEMP,
	CHANGE_TEMP,
	POWER_ON_OFF,
	RESET_CLINT,
};

enum CHOICE {
	NO,
	YES,
};

typedef struct client {
	char id[ID_SIZE_DATA];
	char pass[PASS_SIZE];
	HANDLE handle;
	float temperature;
	float humidity;
	bool status;
	struct tm timestamp;
}DATA;



CRITICAL_SECTION g_cs;//排他制御の宣言
DWORD WINAPI port_monitoring_thread(LPVOID pParam);//挿抜監視用の別スレッド


int  main_screen();//管理選択肢のメイン画面を表示する。
void run_order_menu();//対象のクライアントへの命令に合わせた分岐を行う。
int  order_choice_screen();//クライアントへ送信する命令選択画面。

void error_reaction(int mode);//設定したモードのリアクションを表示する。
void show_log_file();//起動時に直近のログ情報を外部フォルダから取得する。
void write_log_file(DATA* temp);//対象の情報を外部フォルダに書き込む。

int show_file_contents(FILE* address_file, char temp_addr_datas[ALL_CLIENTS][ADDRESS_DATA]);//外部フォルダに保存された情報を一覧表示する。
void get_file_contents(DATA* temp);//外部フォルダから必要な情報を獲得する。

void setup_connection(DATA* temp);//シリアル通信機能のセットを行う。
void send_order(int choice, int send_data, DATA* temp);//設定に従い指定したデータを送信する。
void receive_data(int choice, DATA* temp);//データを受信する。
void close_connection(DATA* temp_struct);//送受信バッファの情報を消去した後、構造体に保持されている情報を初期化する。

int input_check(int lowest, int highest);//上下限のある入力の際に入力内容が範囲内かチェックする。
void get_time(DATA* temp);//現在時刻を取得する

void set_deviceport_lighathouse(HWND hwnd);//OSにシリアルポートに挿抜の検出を要請する。
bool make_massage_window(HINSTANCE hInst);//検出時の通知を受け取る為のウインドウを作る。
HWND setup_massage_window(HINSTANCE  hInst);//作成したウィンドウのセットアップを行う。
void message_waiting();//挿抜が検出されるまでの動作を定めている。
void set_port_monitoring();//検出の要請から待機状態までが格納されている。

void pingpong_address_file(DATA* temp);//与えられたアドレスに対し、通信確認のポーリングを行う。
void remove_address();//外部アドレスから接続が無効なモノを消去する。
void get_add_inport_device(PDEV_BROADCAST_DEVICEINTERFACE base);//OSからの通知で機器の挿入に決まった動作を返す。
void get_com_port_from_path(const wchar_t* device_path, char port_name[ID_SIZE_DATA], char mech_name[ADDRESS_DATA]);//挿入された時の情報を基に使用ポート情報を取得する。
void add_addresFile(char port_name[ID_SIZE_DATA], char mech_name[ADDRESS_DATA]);//接続情報を外部フォルダに記憶する。


void show_status(char temp_data[LOG_SIZE], DATA* temp);//指定したクライアントからセンサーの値を取得し、外部フォルダに保存する事が出来る。
void change_temp(DATA* temp);//指定したクライアントの設定温度を変更する。
void show_temp(char temp_data[LOG_SIZE], DATA* temp);//指定したクライアントから設定温度を取得し、表示する。
void receive_react(int choice, char temp_data[LOG_SIZE], DATA* temp);//受信時のリアクションが内包されたラッパー関数。

void reset_target_client(DATA* temp);//対象のクライアントを再起動させる

//ＯＳからメッセージを受信した際走るプログラム
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_DEVICECHANGE) {
		if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {//挿抜に限定す
			if (wParam == DBT_DEVICEARRIVAL) {//挿
				PDEV_BROADCAST_HDR temp = (PDEV_BROADCAST_HDR)lParam;//ポインタを通知構造体に"
				if (temp != NULL && temp->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
					PDEV_BROADCAST_DEVICEINTERFACE base = (PDEV_BROADCAST_DEVICEINTERFACE)temp;//ポインタを詳細構造体に
					get_add_inport_device(base);
					return TRUE;
				}
			}
			else remove_address(); //抜
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void main(void) {
	printf("外部フォルダを確認\n");
	printf("直近のデータを表示\n");
	show_log_file();
	InitializeCriticalSection(&g_cs);
	remove_address();
	int choice = 0;





	HANDLE hThread = CreateThread(NULL, 0, port_monitoring_thread, NULL, 0, NULL);
	if (hThread != NULL) {
		CloseHandle(hThread);
	}

	bool end_flag = true;
	while (end_flag != false) {

		choice = INPUT_CHECK;
		choice = main_screen();

		switch (choice) {


		case SHOW_LOG:
			show_log_file();
			break;

		case SEND_ORDER:
			run_order_menu();
			break;

		default:
			end_flag = false;
			break;

		}
	}
}

//管理選択肢のメイン画面を表示する。
int main_screen() {
	int choice = 0;


	printf("中央管理画面です、ご希望の操作をお選び下さい\n");
	printf("\n");
	printf("0:データログの確認\n");
	printf("1:クライアントへの命令\n");
	printf("それ以外:管理画面の終了\n");

	choice = input_check(SHOW_LOG, SEND_ORDER);
	rewind(stdin);
	return(choice);
}

//対象のクライアントへの命令に合わせた分岐を行う。
void run_order_menu() {
	int choice = INPUT_CHECK;
	DATA temp_struct = { 0 };

	choice = order_choice_screen();
	if (choice == INPUT_CHECK) {
		error_reaction(INPUT_SYS);
		return;
	}

	get_file_contents(&temp_struct);
	if (temp_struct.pass[0] == '\0') return;
	setup_connection(&temp_struct);

	switch (choice) {

	case SHOW_SENSOR_STATUS:
	case SHOW_TEMP:
	case POWER_ON_OFF:
		send_order(choice, UNUSED, &temp_struct);
		receive_data(choice, &temp_struct);
		break;

	case CHANGE_TEMP:
		send_order(GET_TEMP, UNUSED, &temp_struct);
		receive_data(choice, &temp_struct);
		if (temp_struct.temperature >= 0) {
			send_order(choice, temp_struct.temperature, &temp_struct);
		}
		break;

	case RESET_CLINT:
		reset_target_client(&temp_struct);
		break;

	default:break;
	}
	close_connection(&temp_struct);
}

//クライアントへ送信する命令選択画面。
int order_choice_screen() {
	printf("クライアントに指示する内容を選択してください\n");
	printf("\n");
	printf("0:センサーの確認\n");
	printf("1:設定温度の確認\n");
	printf("2:設定温度の変更\n");
	printf("3:電源のON/OFF\n");
	printf("4:クライアントの再起動\n");
	printf("それ以外:管理画面に戻る\n");

	int choice = input_check(SHOW_SENSOR_STATUS, RESET_CLINT);
	if (choice < SHOW_SENSOR_STATUS || RESET_CLINT < choice) {
		return(INPUT_CHECK);
	}

	return(choice);
}

//設定したモードのリアクションを表示する。
void error_reaction(int mode) {

	switch (mode) {

	case FILE_SYS:
		printf("外部フォルダへのアクセスに失敗しました\n");
		break;

	case COMM_SYS:
		printf("コマンドの送受信に失敗しました\n");
		break;

	case INPUT_SYS:
		printf("正常な入力を受け取れませんでした\n");
		break;

	case STRUCT_SYS:
		printf("ログの保存に失敗しました\n");
		break;

	case WINDOWS_SYS:
		printf("OSへのアプローチに失敗しました\n");
		break;
	case PINGPONG_SYS:
		printf("クライアントとの通信状況の確認に失敗しました\n");
		break;

	case CONNECT_SYS:
		printf("非接続状態\n");
		break;
	default:break;
	}
}

//起動時に直近のログ情報を外部フォルダから取得する。
void show_log_file() {
	FILE* fp;
	char temp_header[LOG_SIZE] = { 0 };



	fp = fopen("Client_Log.txt", "rb");
	if (fp == NULL) {
		error_reaction(FILE_SYS);
		return;
	}

	while (fgets(temp_header, LOG_SIZE, fp) != NULL) {
		printf("%s", temp_header);
	}

	fclose(fp);
}

//対象の情報を外部フォルダに書き込む。
void write_log_file(DATA* temp) {
	FILE* fp;
	char temp_header[LOG_SIZE] = { 0 };

	fp = fopen("Client_Log.txt", "ab");
	if (fp == NULL) {
		error_reaction(FILE_SYS);
		return;
	}
	get_time(temp);
	fprintf(fp, "%s,%.1f,%.1f,%d,%d,%d,%d,%d\n",
		temp->id,
		temp->temperature,
		temp->humidity,
		(temp->timestamp.tm_year + 1900),
		(temp->timestamp.tm_mon + 1),
		temp->timestamp.tm_mday,
		temp->timestamp.tm_hour,
		temp->timestamp.tm_min);
	fclose(fp);
}

//外部フォルダに保存された情報を一覧表示する。
int show_file_contents(FILE* address_file, char temp_addr_datas[ALL_CLIENTS][ADDRESS_DATA]) {
	int count = 0;

	while (count < ALL_CLIENTS && fgets(temp_addr_datas[count], ADDRESS_DATA, address_file) != NULL) {
		printf("%d : %s", count + 1, temp_addr_datas[count]);
		count++;
	}
	rewind(address_file);
	return count;
}

//外部フォルダから必要な情報を獲得する。
void get_file_contents(DATA* temp) {
	char  address_data[ADDRESS_DATA] = { 0 };
	char temp_addr_datas[ALL_CLIENTS][ADDRESS_DATA] = { 0 };
	char* id = NULL;
	char* port = NULL;
	int count = 0;
	int choice = INPUT_CHECK;
	FILE* fp;

	EnterCriticalSection(&g_cs);

	fp = fopen("Client_address.txt", "rb");
	if (fp == NULL) {
		error_reaction(COMM_SYS);
		LeaveCriticalSection(&g_cs);
		return;
	}
	count = show_file_contents(fp, temp_addr_datas);


	fclose(fp);
	LeaveCriticalSection(&g_cs);

	if (count == 0) {
		printf("接続中のクライアントが存在しません\n");//クライアントと非接続状態の際、危険な状態に繋がらない様に弾く
		return;
	}

	printf("対象のクライアントを選択して下さい\n");
	choice = input_check(1, count);
	if (choice == INPUT_CHECK) {
		error_reaction(INPUT_SYS);
		return;
	}

	strcpy(address_data, temp_addr_datas[choice - 1]);

	id = strtok(address_data, ",\r\n");
	port = strtok(NULL, ",\r\n");
	if (id == NULL || port == NULL)
		return;

	strcpy(temp->pass, port);
	strcpy(temp->id, id);

}

//シリアル通信機能のセットを行う。
void setup_connection(DATA* temp) {
	HANDLE client;

	client = CreateFileA(
		temp->pass,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);
	if (client == INVALID_HANDLE_VALUE) {
		temp->handle = NULL;
		return;
	}

	DCB dcb = { 0 };
	dcb.DCBlength = sizeof(DCB);
	GetCommState(client, &dcb);
	dcb.BaudRate = CBR_9600;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;//この一文で明示的に指示されないDTRに対する動作を停止
	SetCommState(client, &dcb);

	EscapeCommFunction(client, CLRDTR);//ポートに待機状態を明示的に指示する事で安定

	COMMTIMEOUTS time_out = { 0 };
	GetCommTimeouts(client, &time_out);
	time_out.ReadIntervalTimeout = 50;
	time_out.ReadTotalTimeoutMultiplier = 1;
	time_out.ReadTotalTimeoutConstant = 1000;

	time_out.WriteTotalTimeoutMultiplier = 0;
	time_out.WriteTotalTimeoutConstant = 1000;

	SetCommTimeouts(client, &time_out);

	temp->handle = client;
}

// 設定に従い指定したデータを送信する。
void send_order(int choice, int send_data, DATA* temp) {
	uint8_t order[2] = { choice,send_data };
	DWORD result = 0;


	if (temp->handle != NULL &&
		temp->handle != INVALID_HANDLE_VALUE) {
		WriteFile(temp->handle, order, 2, &result, NULL);
		if (result == 2) {
			printf("\n");
		}
		else {
			error_reaction(CONNECT_SYS);
			temp->status = false;
			printf("エラー: %d\n", GetLastError());
		}
		printf("SEND=%d\n", order[0]);
	}
	else error_reaction(CONNECT_SYS);


}

//データを受信する。
void receive_data(int choice, DATA* temp) {
	char answer[LOG_SIZE] = { 0 };
	char temp_data[LOG_SIZE] = { 0 };
	int ping_success = false;
	DWORD resurt = 0;
	int ret;



	if (temp->handle != NULL &&
		temp->handle != INVALID_HANDLE_VALUE) {
		if (ReadFile(temp->handle, answer, sizeof(answer), &resurt, NULL) != 0) {
			printf("\n");
		}
		else printf("エラー番号: %d\n", GetLastError());


		ret = sscanf(answer, "%28[^,],%d", temp_data, &ping_success);

		if (ret != 2) {
			error_reaction(COMM_SYS);
			ping_success = false;
		}
		if (ping_success == false) {
			printf("%s\n", temp_data);
			return;
		}
		receive_react(choice, temp_data, temp);
	}
	else error_reaction(CONNECT_SYS);
}

//送受信バッファの情報を消去した後、構造体に保持されている情報を初期化する。
void close_connection(DATA* temp) {
	if (temp->handle != NULL &&
		temp->handle != INVALID_HANDLE_VALUE) {

		PurgeComm(temp->handle, PURGE_RXCLEAR);
		PurgeComm(temp->handle, PURGE_TXCLEAR);
		CloseHandle(temp->handle);
	}
	*temp = (DATA){ 0 };
}

//上下限のある入力の際に入力内容が範囲内かチェックする。
int input_check(int lowest, int highest) {
	int button;
	int input_check;

	input_check = scanf_s("%d", &button);
	rewind(stdin);
	int scan_check = (input_check != 1);
	int numbers_check = (button < lowest || highest < button);


	if (scan_check || numbers_check) {
		button = INPUT_CHECK;
	}
	return(button);
}

//現在時刻を取得する
void get_time(DATA* temp) {
	time_t now = time(NULL);
	temp->timestamp = *localtime(&now);
}

//OSにシリアルポートに挿抜の検出を要請する。
void set_deviceport_lighathouse(HWND hwnd) {
	DEV_BROADCAST_DEVICEINTERFACE lighthouse = { 0 };
	lighthouse.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	lighthouse.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	lighthouse.dbcc_classguid = GUID_DEVINTERFACE_COMPORT;
	HDEVNOTIFY moby_d = RegisterDeviceNotification(
		hwnd,
		&lighthouse,
		DEVICE_NOTIFY_WINDOW_HANDLE
	);
	if (moby_d == NULL) {
		error_reaction(WINDOWS_SYS);
		return;
	}
}

//検出時の通知を受け取る為のウインドウを作る。
bool make_massage_window(HINSTANCE hInst) {

	BOOL InitWindowClass(HINSTANCE  hwnd);
	WNDCLASSEX wc = { 0 };

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInst;
	wc.lpszClassName = TEXT("lighthouce");

	if (!RegisterClassEx(&wc)) {
		return FALSE;
	}
	return TRUE;
}

//作成したウィンドウのセットアップを行う。
HWND setup_massage_window(HINSTANCE  hInst) {

	HWND hwnd = CreateWindowEx(
		0,
		TEXT("lighthouce"),
		NULL,
		0,
		0, 0, 0, 0,
		HWND_MESSAGE,
		NULL,
		hInst,
		NULL
	);

	if (hwnd == NULL) {
		error_reaction(WINDOWS_SYS);
		return NULL;
	}

	return hwnd;
}

//挿抜が検出されるまでの動作を定めている。
void message_waiting() {
	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

//検出の要請から待機状態までが格納されている。
void set_port_monitoring() {
	HINSTANCE hInst = GetModuleHandle(NULL);
	HWND hwnd = { 0 };

	if (!make_massage_window(hInst)) {
		return;
	}
	hwnd = setup_massage_window(hInst);
	if (hwnd == NULL) {
		return;
	}
	set_deviceport_lighathouse(hwnd);
	message_waiting();
}

//検出の要請から待機状態までが格納されている。
DWORD WINAPI port_monitoring_thread(LPVOID pParam) {
	set_port_monitoring();
	return 0;
}

//与えられたアドレスに対し、通信確認のポーリングを行う。
void pingpong_address_file(DATA* temp) {

	setup_connection(temp);
	if (temp->handle == NULL ||
		temp->handle == INVALID_HANDLE_VALUE) {
		printf("%s : ", temp->pass);
		error_reaction(CONNECT_SYS);// クライアントの立ち上がりが送れる等した際に通知を送り、接続状態を知らせる
		return;
	}
	printf("%s : 接続状態\n", temp->pass);
	Sleep(2000);
	PurgeComm(temp->handle, PURGE_RXCLEAR);


	send_order(CONNECT_TEST, UNUSED, temp);
	receive_data(CONNECT_TEST, temp);



	PurgeComm(temp->handle, PURGE_TXCLEAR);
	CloseHandle(temp->handle);
}

//外部アドレスから接続が無効なモノを消去する。
void remove_address() {
	char  address_data[MAX_SIZE] = { 0 };
	FILE* copy = NULL;
	FILE* base = NULL;
	DATA  temp = { 0 };

	char* checker = NULL;
	char* id = NULL;
	char* port = NULL;


	EnterCriticalSection(&g_cs);
	if (fopen_s(&base, "Client_address.txt", "rb") != 0 ||
		fopen_s(&copy, "temp.txt", "ab") != 0) {
		if (base != NULL) { fclose(base); }
		if (copy != NULL) { fclose(copy); }
		error_reaction(FILE_SYS);
		LeaveCriticalSection(&g_cs);
		return;
	}
	while (fgets(address_data, sizeof(address_data), base) != NULL) {
		id = strtok(address_data, ",\r\n");
		port = strtok(NULL, ",\r\n");
		if (id == NULL || port == NULL) {
			fclose(base);
			fclose(copy);
			LeaveCriticalSection(&g_cs);
			return;
		}
		strcpy(temp.pass, port);
		strcpy(temp.id, id);


		pingpong_address_file(&temp);
		if (temp.status == true) {
			fprintf(copy, "%s,%s\n", temp.id, temp.pass);
		}
		temp = (DATA){ 0 };
	}

	fclose(base);
	fclose(copy);


	remove("Client_address.txt");

	if (rename("temp.txt", "Client_address.txt") != 0) {
		perror("Client_address.txtが消えてしまった");
	}
	LeaveCriticalSection(&g_cs);

}

//OSからの通知で機器の挿入に決まった動作を返す。
void get_add_inport_device(PDEV_BROADCAST_DEVICEINTERFACE base) {
	char port_name[MAX_SIZE] = { 0 };
	char mech_name[MAX_SIZE] = { 0 };

	get_com_port_from_path(base->dbcc_name, port_name, mech_name);
	printf("%s : 接続状態\n", port_name); //サブウィンドウの動作に付随し、接続した際に知らせる
	add_addresFile(port_name, mech_name);
}

//挿入された時の情報を基に使用ポート情報を取得する。
void get_com_port_from_path(const wchar_t* device_path, char port_name[ID_SIZE_DATA], char mech_name[ADDRESS_DATA]) {

	if (!device_path) return;

	HDEVINFO hDevInfo = SetupDiGetClassDevs(//情報へのアクセスハンドル取得
		&GUID_DEVINTERFACE_COMPORT,//現在接続中のPORTの情報指定
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (hDevInfo == INVALID_HANDLE_VALUE) return;

	SP_DEVICE_INTERFACE_DATA ifData = { 0 };
	ifData.cbSize = sizeof(ifData);

	if (!SetupDiOpenDeviceInterfaceW(//上位関数で見つけてきたデバイス情報を基に直で呼び出す
		hDevInfo,
		device_path,//挿抜に関わる接続の情報
		0,//対象のサイズが分からないのでここは一旦０にしている
		&ifData))
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return;
	}

	DWORD required = 0;
	SetupDiGetDeviceInterfaceDetailW(
		hDevInfo,
		&ifData,
		NULL,
		0,
		&required,//OSが必要なサイズを格納してくれるからそれを使う
		NULL);

	PSP_DEVICE_INTERFACE_DETAIL_DATA_W detail = //////接続系の構造体
		(PSP_DEVICE_INTERFACE_DETAIL_DATA_W)malloc(required);

	if (!detail) {
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return;
	}

	detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

	SP_DEVINFO_DATA devData = { 0 };
	devData.cbSize = sizeof(devData);//ここで更に詳細な情報を受け取れる構造体に入れ込んでいる
	if (SetupDiGetDeviceInterfaceDetailW(
		hDevInfo,
		&ifData,
		detail,
		required,
		NULL,
		&devData))
	{
		HKEY hKey = SetupDiOpenDevRegKey(//OSの機器情報部分に触れるためのハンドル
			hDevInfo,
			&devData,
			DICS_FLAG_GLOBAL,
			0,
			DIREG_DEV,
			KEY_READ);

		if (hKey != INVALID_HANDLE_VALUE) {
			DWORD size = MAX_SIZE;
			if (RegQueryValueExA(hKey, "PortName", NULL, NULL, (LPBYTE)port_name, &size) == ERROR_SUCCESS) {
				SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)mech_name, MAX_SIZE, NULL);

				char* p = strstr(mech_name, " (COM");
				if (p) *p = '\0';

				char temp[MAX_SIZE] = { 0 };
				for (char* src = mech_name, *dst = temp; *src; src++) {
					if (*src != '\n') *dst++ = *src;
				}
				strcpy(mech_name, temp);
			}
			RegCloseKey(hKey);
		}
	}
	free(detail);
	SetupDiDestroyDeviceInfoList(hDevInfo);
}

;//接続情報を外部フォルダに記憶する。
void add_addresFile(char port_name[ID_SIZE_DATA], char mech_name[ADDRESS_DATA]) {
	FILE* fp;

	EnterCriticalSection(&g_cs);
	fp = fopen("Client_address.txt", "a");
	if (fp == NULL) {
		error_reaction(FILE_SYS);
		LeaveCriticalSection(&g_cs);
		return;
	}
	fprintf(fp, "%s,%s\n", mech_name, port_name);
	LeaveCriticalSection(&g_cs);

	fclose(fp);
}

//指定したクライアントからセンサーの値を取得し、外部フォルダに保存する事が出来る。
void show_status(char temp_data[LOG_SIZE], DATA* temp) {
	int temp_choice;
	int ret;


	printf("%s\n", temp_data);
	printf("ログに保存しますか？\n");
	printf("yes : 1\n");
	printf("no  : 0\n");


	temp_choice = input_check(NO, YES);
	if (temp_choice < NO || YES < temp_choice) {
		error_reaction(INPUT_SYS);
		return;
	}
	if (temp_choice == 0) {
		return;
	}
	else {
		ret = sscanf(temp_data, "humid:%f-temper:%f", &temp->humidity, &temp->temperature);
		if (ret == 2) {
			write_log_file(temp);
		}
		else {
			error_reaction(STRUCT_SYS);
		}
	}

}

//指定したクライアントの設定温度を変更する。
void change_temp(DATA* temp) {

	printf("対象の設定温度を変更致します\n");
	printf("0度から９９度までしか設定出来ません\n");

	int tempf = input_check(0, 99);
	if (tempf < 0 || 99.0 < tempf) {
		error_reaction(INPUT_SYS);
		temp->temperature = INPUT_CHECK;
		return;
	}
	temp->temperature = tempf;
}

//指定したクライアントから設定温度を取得し、表示する。
void show_temp(char temp_data[LOG_SIZE], DATA* temp) {
	int ret;

	ret = sscanf(temp_data, "%f", &temp->temperature);
	if (ret != 1) {
		error_reaction(STRUCT_SYS);
	}
	printf("現在の設定温度: %.1f\n", temp->temperature);

}

//受信時のリアクションが内包されたラッパー関数。
void receive_react(int choice, char temp_data[LOG_SIZE], DATA* temp) {
	switch (choice) {

	case SHOW_SENSOR_STATUS:
		show_status(temp_data, temp);
		break;

	case SHOW_TEMP:
		show_temp(temp_data, temp);
		break;

	case CHANGE_TEMP:
		show_temp(temp_data, temp);
		change_temp(temp);
		break;

	case PING_PONG:
		temp->status = true;
		break;

	default:return;
	}


}

//対象のクライアントを再起動させる
void reset_target_client(DATA* temp) {
	if (temp == NULL || temp->handle == NULL || temp->handle == INVALID_HANDLE_VALUE) {
		return;
	}

	HANDLE client = temp->handle;

	EscapeCommFunction(client, SETDTR);
	Sleep(100);
	EscapeCommFunction(client, CLRDTR);
	Sleep(1500);
}


