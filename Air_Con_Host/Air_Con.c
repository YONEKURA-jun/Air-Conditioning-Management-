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
#define LOG_SIZE 16 
#define ID_SIZE  5
#define PORT_SIZE 6
#define ADDRESS_DATA 15


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
void set_File_send();//送受信のセットアップを行う関数


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
			set_File_send();
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

void get_log() {
	FILE* fp;
	char temp_header[HEADER_SIZE] = { 0 };

	fp = fopen("Client_Log.txt", "rb");
	if (fp == NULL) {
		error_reaction(FILE_SYS);
		return;
	}
	fgets(temp_header, HEADER_SIZE, fp);
	printf("%s\n", temp_header);


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

void set_File_send() {///todo: 標準入力での対象の指定　不正な入力への対策　送信内容の設定　　関数化して昨日を単純化
	char  address_data[ADDRESS_DATA] = { 0 };
	char*   id = NULL;
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

	printf("対象：%s番に命令を送ります、よろしいですか？\n", id);
	printf("テスト用プリント文：%s\n", port);
	printf("\n");

}

