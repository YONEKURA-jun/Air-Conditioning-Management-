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
#define PORT_SIZE 6
#define ADDRESS_DATA 15
#define CHECK_DATA 6


enum MAIN_SCREEN {
	SHOW_LOG,
	CLIENT_CONTROL,
};

enum ERROR_REACTION {
	FILE_SYS,
	COMM_SYS,
};


typedef struct client {
	uint8_t ID;
	uint8_t temperature;
	uint8_t humidity;
	bool status;
	time_t timestamp;
}DATA;


int main_screen();//管理選択肢のメイン画面を表示する。
void get_log();//起動時に直近６時間分の情報を外部フォルダから取得する。
void error_reaction(int mode);//設定したモードのリアクションを表示する。

void get_File_addr();//外部フォルダから必要な情報を獲得する。
void set_up_send(char* port);//引数の外部機器との通信機能のセットを行う。
void make_send_data(HANDLE handle);//設定に従い指定したデータを送信する。
void standby_catch_data(HANDLE handle);//受信したデータを画面に表示する。


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
			get_File_addr();
			break;

		default:
			end_flag = false;
		}
	}
}

int main_screen() {
	int button = 0;


	printf("中央管理画面です、ご希望の操作をお選び下さい\n");
	printf("\n");
	printf("0:データログの確認\n");
	printf("1:クライアントへの命令\n");
	printf("それ以外:管理画面の終了\n");


	if (scanf("%d", &button) != 1) {
		button = INPUT_CHECK;
	}
	rewind(stdin);
	return(button);
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

void error_reaction(int mode) {

	switch (mode) {

	case FILE_SYS:
		printf("フォルダへのアクセスに失敗しました");
		break;

	case COMM_SYS:
		printf("コマンドの送受信に失敗しました");
		break;

	default:
		break;
	}
}

void get_File_addr() {
	char  address_data[ADDRESS_DATA] = { 0 };
	char* id = NULL;
	char* port = NULL;
	int checker = INPUT_CHECK;
	FILE* Client_address;


	Client_address = fopen("Client_address.txt", "rb");
	if (Client_address == NULL) {
		error_reaction(COMM_SYS);
		return;
	}


	fgets(address_data, sizeof(address_data), Client_address);
	fclose(Client_address);
	id = strtok(address_data, ",\n");
	port = strtok(NULL, ",\n");


	printf("対象：%s\n", id);
	printf("\n");
	set_up_send(port);

}

void set_up_send(char* port) {
	HANDLE client;

	client = CreateFileA(
		port,
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

	make_send_data(client);
	standby_catch_data(client);
	CloseHandle(client);

}

void make_send_data(HANDLE handle) {
	char order[] = "0";
	DWORD result = 0;

	WriteFile(handle, order, 1, &result, NULL);
	if (result == 1) {
		printf("送信に成功しました！\n");
	}
	else  printf("エラー番号: %d\n", GetLastError());
}

void standby_catch_data(HANDLE handle) {
	char answer[LOG_SIZE] = { 0 };
	DWORD resurt = 0;
	if (ReadFile(handle, answer, sizeof(answer), &resurt, NULL) != 0) {
		printf("受信に成功しました！\n");
	}
	else printf("エラー番号: %d\n", GetLastError());
	printf("%s", answer);
}