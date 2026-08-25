//HOST
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define INPUT_CHECK -1
#define CLIENT_NAME 21
#define ALL_CLIENTS 15



//画面表示する際には外部データには可動状態・各センサの管理名も合わせて表示
enum MAIN_SCREEN {
	SHOW_LOG,
	CLIENT_CONTROL,
};

typedef struct client{
	int ID;         //外部にアドレスのフォルダを作って、通信する際に参照する
	char client[CLIENT_NAME];
	int temperature;
	int humidity;
	bool stutas;    //maybe:稼働状態？　更新が無かった場合これも記録に残す？ 
	long time;      //年・月・日
}DATAS;


int main_screen();//管理選択肢のメイン画面を表示する。
//void get_log();//起動時に直近６時間分の情報を外部フォルダから取得する。







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
			break;

		case CLIENT_CONTROL:
			break;

		default:
			end_flag = false;
		}
	}
}

int main_screen() {
	int button = INPUT_CHECK;
	int checker = 0;

	printf("中央管理画面です、ご希望の操作をお選び下さい");
	printf("\n");
	printf("1:データログの確認");
	printf("2:クライアントへの命令");
	printf("それ以外:管理画面の終了");

	if (scanf("%d", &button) != 1) {
		printf("入力に失敗した");
	}
	rewind(stdin);
	return(button);
}

//void get_log() {}