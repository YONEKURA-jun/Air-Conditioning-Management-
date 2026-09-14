//HOST
#define _CRT_SECURE_NO_WARNINGS


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <Windows.h>
#include <string.h>

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
#define MAX_PORT_NAME 32

enum MAIN_SCREEN {
	SHOW_LOG,
	CLIENT_CONTROL,
};

enum CLIENT_SCREEN {
	SHOW_STATUS,
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

enum ORDER {
	CHANGE_TEMP = 1,
	POWER_ON_OFF = 2,
	PING_PONG = 255,
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

DATA g_recive_data = { 0 };
char g_connected_devices[ID_SIZE_DATA] = { 0 };
bool g_add_addres_flag = false;


CRITICAL_SECTION g_cs;//排他制御の宣言
DWORD WINAPI port_monitoring_thread(LPVOID pParam);//挿抜監視用の別スレッド


int main_screen();//管理選択肢のメイン画面を表示する。
void ToSend_order_screen();//クライアントを対象とした選択画面を表示する。
int order_choice_screen();//クライアントへ送信する命令選択画面。


void show_logFile();//起動時に直近６時間分の情報を外部フォルダから取得する。
void error_reaction(int mode);//設定したモードのリアクションを表示する。

int show_FileContents(FILE* address_file);//外部フォルダに保存された情報を一覧表示する。
void get_FileContents(DATA* temp);//外部フォルダから必要な情報を獲得する。
void skip_FileLines(int targer, FILE* address_file);//対象のファイルポインタを与えられた行数分進める。

void setup_connection(DATA* temp);//シリアル通信機能のセットを行う。
void send_order(int choice, DATA* temp);//設定に従い指定したデータを送信する。
void receive_data(int choice, DATA* temp);//受信したデータを画面に表示する。
void close_connection();//構造体に保持されている情報を初期化する。

int input_check(int lowest, int highest);//上下限のある入力の際に入力内容が範囲内かチェックする。
void get_time(DATA* temp);//現在時刻を取得する

void set_DevicePort_LighatHouse(HWND hwnd);//OSにシリアルポートに挿抜の検出を要請する。
bool make_MassageWindow(HINSTANCE hInst);//検出時の通知を受け取る為のウインドウを作る。
HWND setup_MassageWindow(HINSTANCE  hInst);//作成したウィンドウのセットアップを行う。
void message_waiting();//挿抜が検出されるまでの動作を定めている。
void set_port_monitoring();//検出の要請から待機状態までが格納されている。

void PingPong_address_File(DATA* temp);//与えられたアドレスに対し、通信確認のポーリングを行う。
void remove_address();//外部アドレスから接続が無効なモノを消去する。
void get_add_inport_device(PDEV_BROADCAST_DEVICEINTERFACE base);//OSからの通知で機器の挿入に決まった動作を返す。
void get_com_port_from_path(const wchar_t* device_path, char port_name[MAX_PORT_NAME]);//挿入された時の情報を基に使用ポート情報を取得する。
void add_addresFile(char port_name[ID_SIZE_DATA], bool flag);//使用ポート情報を外部フォルダに記憶する。


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_DEVICECHANGE) {
		if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {//挿抜に限定す

			if (wParam == DBT_DEVICEARRIVAL) {//挿
				PDEV_BROADCAST_HDR temp = (PDEV_BROADCAST_HDR)lParam;//ポインタを通知構造体に
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
	printf("直近六時間のデータを表示\n");
	show_logFile();
	remove_address();

	int choice = 0;

	HANDLE hThread = CreateThread(NULL, 0, port_monitoring_thread, NULL, 0, NULL);
	if (hThread != NULL) {
		CloseHandle(hThread);
	}
	bool end_flag = true;
	InitializeCriticalSection(&g_cs);
	while (end_flag != false) {

		EnterCriticalSection(&g_cs);
		add_addresFile(g_connected_devices, g_add_addres_flag);
		LeaveCriticalSection(&g_cs);

		choice = INPUT_CHECK;
		choice = main_screen();
		switch (choice) {

		case SHOW_LOG:
			show_logFile();
			break;

		case CLIENT_CONTROL:
			ToSend_order_screen();
			break;

		default:
			end_flag = false;
			break;

		}
	}
}

int main_screen() {
	int choice = 0;


	printf("中央管理画面です、ご希望の操作をお選び下さい\n");
	printf("\n");
	printf("0:データログの確認\n");
	printf("1:クライアントへの命令\n");
	printf("それ以外:管理画面の終了\n");

	choice = input_check(SHOW_LOG, CLIENT_CONTROL);
	rewind(stdin);
	return(choice);
}

void ToSend_order_screen() {
	int choice = INPUT_CHECK;

	printf("0:クライアントの現在の状態を知りたい\n");
	printf("1:クライアントへ命令を送る\n");
	printf("それ以外:戻る\n");

	choice = input_check(SHOW_STATUS, SEND_ORDER);
	if (choice < SHOW_STATUS || SEND_ORDER < choice) {
		error_reaction(INPUT_SYS);
		return;
	}

	get_FileContents(&g_recive_data);
	setup_connection(&g_recive_data);

	switch (choice) {

	case SHOW_STATUS:
		send_order(choice, &g_recive_data);
		receive_data(choice, &g_recive_data);
		break;

	case SEND_ORDER:
		choice = order_choice_screen();
		if (choice == INPUT_CHECK) {
			close_connection();
			return;
		}
		send_order(choice, &g_recive_data);
		receive_data(choice, &g_recive_data);
		if (choice == CHANGE_TEMP) {
			send_order(g_recive_data.temperature, &g_recive_data);
		}
		break;

	default: break;
	}
	close_connection();
}

int order_choice_screen() {
	printf("クライアントに指示する内容を選択してください\n");
	printf("1:設定温度の変更\n");
	printf("2:電源のON/OFF\n");
	printf("それ以外:前の画面に戻る\n");

	int choice = input_check(CHANGE_TEMP, POWER_ON_OFF);
	if (choice < CHANGE_TEMP || POWER_ON_OFF < choice) {
		return(INPUT_CHECK);
	}
	return(choice);
}

DWORD WINAPI port_monitoring_thread(LPVOID pParam) {
	set_port_monitoring();
	return 0;
}

void show_logFile() {//todo;
	FILE* fp;
	char temp_header[LOG_SIZE] = { 0 };
	int count = 0;


	fp = fopen("Client_Log.txt", "rb");
	if (fp == NULL) {
		error_reaction(FILE_SYS);
		return;
	}

	while (fgets(temp_header, LOG_SIZE, fp) != NULL && count < CHECK_DATA) {
		printf("%s", temp_header);
		count++;
	}

	fclose(fp);
}

void write_logFile(DATA* temp) {
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

int show_FileContents(FILE* address_file) {
	char  address_data[ADDRESS_DATA] = { 0 };
	int count = 1;

	while (fgets(address_data, sizeof(address_data), address_file) != NULL) {
		printf("%d : %s\n", count, address_data);
		count++;
	}
	rewind(address_file);
	return(count);
}

void get_FileContents(DATA* temp) {
	char  address_data[ADDRESS_DATA] = { 0 };
	char* id = NULL;
	char* port = NULL;
	int count = 0;
	int choice = INPUT_CHECK;
	FILE* fp;

	fp = fopen("Client_address.txt", "rb");
	if (fp == NULL) {
		error_reaction(COMM_SYS);
		return;
	}

	count = show_FileContents(fp);
	printf("対象のクライアントを選択して下さい\n");

	choice = input_check(1, count);
	if (choice == INPUT_CHECK) {
		error_reaction(INPUT_SYS);
		fclose(fp);
		return;
	}

	int target = choice - 1;
	skip_FileLines(target, fp);

	fgets(address_data, sizeof(address_data), fp);
	fclose(fp);

	id = strtok(address_data, ",\r\n");
	port = strtok(NULL, ",\r\n");

	strcpy(temp->pass, port);
	strcpy(temp->id, id);

	printf("対象：%s\n", id);
}

void skip_FileLines(int targer, FILE* address_file) {
	char  temp[ADDRESS_DATA] = { 0 };
	for (int skip = 0; skip < targer; skip++) {
		fgets(temp, sizeof(temp), address_file);
	}
}

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


	DCB dcb = { 0 };
	dcb.DCBlength = sizeof(DCB);
	GetCommState(client, &dcb);
	dcb.BaudRate = CBR_9600;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	SetCommState(client, &dcb);

	COMMTIMEOUTS time_out = { 0 };
	GetCommTimeouts(client, &time_out);
	time_out.ReadIntervalTimeout = 50;
	time_out.ReadTotalTimeoutConstant = 1000;
	SetCommTimeouts(client, &time_out);

	temp->handle = client;

}

void send_order(int choice, DATA* temp) {
	uint8_t order = choice;
	DWORD result = 0;

	WriteFile(temp->handle, &order, 1, &result, NULL);
	if (result == 1) {
		printf("送信\n");
	}
	else {
		error_reaction(CONNECT_SYS);
		temp->status = false;
		printf("エラー: %d\n", GetLastError());
	}
}

void receive_data(int choice, DATA* temp) {
	char answer[LOG_SIZE] = { 0 };
	char temp_data[LOG_SIZE] = { 0 };
	int status = false;
	DWORD resurt = 0;
	int temp_choice;
	int ret;


	if (ReadFile(temp->handle, answer, sizeof(answer), &resurt, NULL) != 0) {
		printf("受信\n");
	}
	else printf("エラー番号: %d\n", GetLastError());



	ret = sscanf(answer, "%[^,],%d", temp_data, &status);
	if (ret != 2) {
		error_reaction(COMM_SYS);
		status = false;
	}
	if (status == false) {
		printf("%s\n", temp_data);
		return;
	}


	switch (choice) {

	case SHOW_STATUS:

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
			ret = sscanf(answer, "humid:%f-temper:%f", &temp->humidity, &temp->temperature);
			if (ret == 2) {
				write_logFile(&g_recive_data);
			}
			else {
				error_reaction(STRUCT_SYS);
			}
		}
		break;

	case CHANGE_TEMP:

		ret = sscanf(temp_data, "%f", &temp->temperature);
		if (ret != 1) {
			error_reaction(STRUCT_SYS);
		}
		printf("現在の設定温度: %.1f", temp->temperature);
		printf("設定温度を変更しますか？\n");
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
			printf("0度から９９度の間で設定して下さい\n");
			int tempf = input_check(0, 99);
			if (tempf < 0 || 99 < tempf) {
				error_reaction(INPUT_SYS);
				return;
			}
			temp->temperature = tempf;
		}
		break;

	case PING_PONG:
		temp->status = true;
		break;

	default:return;
	}
}

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

void close_connection() {
	PurgeComm(g_recive_data.handle, PURGE_RXCLEAR);
	PurgeComm(g_recive_data.handle, PURGE_TXCLEAR);
	CloseHandle(g_recive_data.handle);
	g_recive_data = (DATA){ 0 };
}

void get_time(DATA* temp) {
	time_t now = time(NULL);
	temp->timestamp = *localtime(&now);
}

void set_DevicePort_LighatHouse(HWND hwnd) {
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

HWND setup_MassageWindow(HINSTANCE  hInst) {

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

bool make_MassageWindow(HINSTANCE hInst) {

	BOOL InitWindowClass(HINSTANCE  hwnd);
	WNDCLASSEX wc = { 0 };

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInst;
	wc.lpszClassName = TEXT("lighthouce");

	//(失敗したら 0 が返る）
	if (!RegisterClassEx(&wc)) {
		return FALSE;
	}
	return TRUE;
}

void message_waiting() {
	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void set_port_monitoring() {
	HINSTANCE hInst = GetModuleHandle(NULL);
	HWND hwnd = { 0 };

	if (!make_MassageWindow(hInst)) {
		return;
	}
	hwnd = setup_MassageWindow(hInst);
	if (hwnd == NULL) {
		return;
	}
	set_DevicePort_LighatHouse(hwnd);
	message_waiting();
}

void PingPong_address_File(DATA* temp) {
	setup_connection(temp);
	send_order(CONNECT_TEST, temp);
	receive_data(CONNECT_TEST, temp);
}

void remove_address() {
	char  address_data[ADDRESS_DATA] = { 0 };
	FILE* copy = NULL;
	FILE* base = NULL;
	DATA  temp = { 0 };


	char* checker = NULL;
	char* id = NULL;
	char* port = NULL;

	if (fopen_s(&base, "Client_address.txt", "r") != 0 ||
		fopen_s(&copy, "temp.txt", "a") != 0) {
		if (base != NULL) { fclose(base); }
		if (copy != NULL) { fclose(copy); }
		error_reaction(FILE_SYS);
		return;
	}
	while (fgets(address_data, sizeof(address_data), base) != NULL) {
		id = strtok(address_data, ",\r\n");
		port = strtok(NULL, ",\r\n");
		strcpy(temp.pass, port);
		strcpy(temp.id, id);
		printf("確認対象：%s\n", port);


		PingPong_address_File(&temp);
		if (temp.status == true) {
			fprintf(copy, "%s,%s\n", temp.id, temp.pass);
		}
		CloseHandle(g_recive_data.handle);
		g_recive_data = (DATA){ 0 };
	}

	fclose(base);
	fclose(copy);

	remove("Client_address.txt");

	if (rename("temp.txt", "Client_address.txt") != 0) {
		perror("Client_address.txtが消えてしまった");
	}
}

void get_add_inport_device(PDEV_BROADCAST_DEVICEINTERFACE base) {
	char port_name[MAX_PORT_NAME] = { 0 };
	get_com_port_from_path(base->dbcc_name, port_name);
	printf("新たな機器の接続がありました\n");

}

void get_com_port_from_path(const wchar_t* device_path, char port_name[MAX_PORT_NAME])
{
	if (!device_path) return;//安全チェック

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

		if (hKey != INVALID_HANDLE_VALUE)
		{
			DWORD size = MAX_PORT_NAME;

			if (RegQueryValueExA(
				hKey,
				"PortName",
				NULL,
				NULL,
				(LPBYTE)port_name, &size) == ERROR_SUCCESS)
			{
				EnterCriticalSection(&g_cs);
				strcpy(g_connected_devices, port_name);
				g_add_addres_flag = true;
				LeaveCriticalSection(&g_cs);

				printf("PORT : %s\n", port_name);
			}
			RegCloseKey(hKey);
		}
	}




	free(detail);
	SetupDiDestroyDeviceInfoList(hDevInfo);
}

void add_addresFile(char port_name[ID_SIZE_DATA], bool flag) {
	FILE* fp;
	char address_id[ID_SIZE_DATA] = { 0 };
	int checker = INPUT_CHECK;
	int c;


	if (flag == true) {
		fp = fopen("Client_address.txt", "a");
		if (fp == NULL) {
			error_reaction(FILE_SYS);
			return;
		}
		printf("接続された機器に対して名前を設定してください\n");
		checker = scanf_s("%s", address_id, ID_SIZE_DATA);
		rewind(stdin);

		if (checker != 1) {
			printf("名前が長すぎます、ポート名を流用致します\n");
			strcpy(address_id, port_name);
		}
		fprintf(fp, "%s,%s\n", address_id, port_name);
		fclose(fp);



		g_add_addres_flag = false;
		g_connected_devices[0] = 0;
	}

}


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