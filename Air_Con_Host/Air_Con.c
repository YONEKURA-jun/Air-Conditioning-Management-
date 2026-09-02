//HOST
#define _CRT_SECURE_NO_WARNINGS


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <Windows.h>
#include <string.h>

#define INPUT_CHECK -1
#define CLIENT_NAME 21
#define ALL_CLIENTS 15
#define HEADER_SIZE 26
#define LOG_SIZE 17 
#define ID_SIZE_DATA 6
#define PASS_SIZE 6
#define ADDRESS_DATA 15
#define CHECK_DATA 6


enum MAIN_SCREEN {
	SHOW_LOG,
	CLIENT_CONTROL,
};

enum CLIENT_SCREEN {
	SHOW_STATUS = 1,
	SEND_ORDER = 2,
};

enum ERROR_REACTION {
	FILE_SYS,
	COMM_SYS,
	INPUT_SYS,
};

typedef struct client {
	uint8_t ID;
	char pass[PASS_SIZE];
	HANDLE handle;
	uint8_t temperature;
	uint8_t humidity;
	bool status;
	time_t timestamp;
}DATA;

static DATA g_recive_data = { 0 };


int main_screen();//管理選択肢のメイン画面を表示する。
void ToSend_order_screen();//クライアントを対象とした選択画面を表示する。


void get_log();//起動時に直近６時間分の情報を外部フォルダから取得する。
void error_reaction(int mode);//設定したモードのリアクションを表示する。

int show_FileContents(FILE* address_file);//外部フォルダに保存された情報を一覧表示する。
void get_FileContents();//外部フォルダから必要な情報を獲得する。
void skip_FileLines(int targer, FILE* address_file);//対象のファイルポインタを与えられた行数分進める。

void make_set_up();//シリアル通信機能のセットを行う。
void send_order(uint8_t choice);//設定に従い指定したデータを送信する。
void receive_data();//受信したデータを画面に表示する。
void Init_struct();//構造体に保持されている情報を初期化する。

int input_check(int lowest, int highest);//上下限のある入力の際に入力内容が範囲内かチェックする。

void main(void) {
	printf("外部フォルダを確認\n");
	printf("直近六時間のデータを表示\n");
	get_log();
	int choice = 0;
	bool end_flag = true;

	while (end_flag != false) {
		choice = INPUT_CHECK;
		choice = main_screen();
		switch (choice) {

		case SHOW_LOG:
			get_log();
			break;

		case CLIENT_CONTROL:
			ToSend_order_screen();
			break;
		
		default:
			end_flag = false;
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

	choice = input_check(SHOW_LOG,CLIENT_CONTROL);

	rewind(stdin);
	return(choice);
}

void ToSend_order_screen() {
	int choice = INPUT_CHECK;

	printf("1:クライアントの現在の状態を知りたい\n");
	printf("2:クライアントへ命令を送る\n");
	printf("それ以外:戻る\n");
	printf("\n");

	choice = input_check(SHOW_STATUS, SEND_ORDER);
	if (choice > SHOW_STATUS || SEND_ORDER < choice) {
		return;
	}

	get_FileContents();
	make_set_up();

	switch (choice) {
	case SHOW_STATUS:
		send_order(choice);
		receive_data();
		break;
	case SEND_ORDER:
		printf("未実装");
		break;


	default: break;
	}
	Init_struct();
}

void get_log() {//todo;
	FILE* fp;
	char temp_header[HEADER_SIZE] = { 0 };
	int count = 0;


	fp = fopen("Client_Log.txt", "rb");
	if (fp == NULL) {
		error_reaction(FILE_SYS);
		return;
	}

	while (fgets(temp_header, HEADER_SIZE, fp) != NULL && count < CHECK_DATA) {
		printf("%s\n", temp_header);
	}

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

void get_FileContents() {
	char  address_data[ADDRESS_DATA] = { 0 };
	char* id = NULL;
	char* port = NULL;
	int count = 0;
	int choice = INPUT_CHECK;
	FILE* fp;
	DATA temp = g_recive_data;

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

	strcpy(temp.pass, port);


	g_recive_data = temp;
	printf("対象：%s\n", id);
	printf("\n");
}

void skip_FileLines(int targer, FILE* address_file) {
	char  temp[ADDRESS_DATA] = { 0 };
	for (int skip = 0; skip < targer; skip++) {
		fgets(temp, sizeof(temp), address_file);
	}
}

void make_set_up() {
	HANDLE client;
	DATA temp = g_recive_data;

	client = CreateFileA(
		temp.pass,
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

	temp.handle = client;
	g_recive_data = temp;
}

void send_order(uint8_t choice) {
	uint8_t order = choice;
	DWORD result = 0;
	DATA temp = g_recive_data;

	WriteFile(temp.handle, &order, 1, &result, NULL);
	if (result == 1) {
		printf("送信に成功\n");
	}
	else  printf("エラー: %d\n", GetLastError());
}

void receive_data() {
	char answer[LOG_SIZE] = { 0 };
	DWORD resurt = 0;
	DATA temp = g_recive_data;

	if (ReadFile(temp.handle, answer, sizeof(answer), &resurt, NULL) != 0) {
		printf("受信に成功\n");
	}
	else printf("エラー番号: %d\n", GetLastError());
	printf("%s", answer);
}

int input_check(int lowest, int highest) {
	int button;
	int input_check;

	input_check = scanf_s("%d", &button);
	rewind(stdin);
	int scan_check = (input_check != 1);
	int numbers_check = (button < lowest || button > highest);


	if (scan_check || numbers_check) {
		button = INPUT_CHECK;
	}
	return(button);
}

void error_reaction(int mode) {

	switch (mode) {

	case FILE_SYS:
		printf("フォルダへのアクセスに失敗しました\n");
		break;

	case COMM_SYS:
		printf("コマンドの送受信に失敗しました\n");
		break;

	case INPUT_SYS:
		printf("正常な入力を受け取れませんでした\n");
		break;

	default:
		break;
	}
}

void Init_struct() {
	CloseHandle(g_recive_data.handle);
	g_recive_data = (DATA){ 0 };
}
