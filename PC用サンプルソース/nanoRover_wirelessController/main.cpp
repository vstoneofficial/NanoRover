/*

	ナノローバー 無線通信(WiFi/Bluetooth)サンプルソース	(c)Vstone	2019.7.23
	
	本ソースは、ナノローバーの無線通信サンプルをPCから制御する場合のC言語サンプルソースです。
	対応開発環境:Visual Studio 2015以降(VC++)

	プログラムの実行前に、ナノローバーに無線通信用のプログラムを書き込んでください。
	続いてbluetoothの場合はPCとペアリングする、Wi-Fiの場合はPCと同じLAN内のアクセスポイントに接続して、PCからナノローバーに通信できる状態にしておいてください。
	また、Bluetoothの場合はCOMポート番号、Wi-Fiの場合はIPアドレスをそれぞれ確認し、COMまたはIP_ADDRESSのマクロを適宜書き換えてください。

*/

#include	<stdio.h>
#include	<windows.h>
#include	<Wininet.h>



#pragma	comment(lib,"ws2_32.lib")

//WiFi通信の際に使用するナノローバーのIPアドレス(実行環境に応じて書き換え)
#define	IP_ADDRESS	("192.168.1.92")

//bluetooth通信の際に使用するナノローバーのCOMポート番号(実行環境に応じて書き換え)
#define	COM	("\\\\.\\COM62")

//汎用通信バッファサイズ
#define	BUFF_SIZE	(1024)

//通信方式の列挙子
enum COMMUNICATION_TYPE { WIFI = 0, BLUETOOTH = 1 };

//デフォルトの通信方式の設定
int communication_type = BLUETOOTH;

int read_encoder();
int set_LED_color(int ch, unsigned char r, unsigned char g, unsigned char b);
int set_wheel_speed(int l, int r);
int clear_encoder();
int set_pen_updown(int is_up);

/*
	main関数
	CUIベースでコマンドを入力し、ナノローバーを制御する。
	
	入力例:	P 1\n … ペンアップ
			S -5 5\n … 5mm/secの速度で前進
			L 0 255 0 0\n … 頭頂のLEDを赤(R255,G0,B0)に変更
			N 0\n … 通信方法をWiFiに変更(デフォルトの通信設定は、int communication_typeの初期値に従う)
*/
int main(int argc, char *argv[]) {

	int loop = 1;
	printf("please input any command.\n");
	printf("E:read encoder.\n");
	printf("C:clear encoder.\n");
	printf("S:set wheel speed.\n");
	printf("P:pen up&down.\n");
	printf("L:set LED color.\n");
	printf("N:select communication type.\n");
	printf("Q:exit program.\n");

	while (loop) {
		char cmd;
		int err = 0;
		printf(">");
		scanf_s("%c", &cmd, 1);

		switch (cmd) {
		case 'e':
		case 'E':
			{
				printf("read encoder.\n");
				err = read_encoder();
			}
			break;
		case 'c':
		case 'C':
			printf("clear encoder.\n");
			err = clear_encoder();
			break;
		case 's':
		case 'S':
			{
				int l, r;
				printf("set wheel speed.\n");
				printf("please input left and right wheel speed( -2147483648 to 2147483647 ).\n");
				scanf_s("%d %d", &l, &r);
				err = set_wheel_speed(l, r);
			}
			break;
		case 'p':
		case 'P':
			{
				char is_up;
				printf("pen updown.\n");
				printf("please input 0 or 1(down/up).\n");
				scanf_s("%hhd", &is_up);
				err = set_pen_updown(is_up);
			}
			break;
		case 'l':
		case 'L':
			{
				int ch;
				unsigned char r, g, b;
				printf("set LED.\n");
				printf("please input LED ch( 0 to 8).\n");
				scanf_s("%d", &ch);
				printf("please input color r,g,b( 0 to 255).\n");
				scanf_s("%hhd %hhd %hhd", &r, &g, &b);

				err = set_LED_color(ch, r, g, b);
			}
			break;
		case 'n':
		case 'N':
			{
				int type;
				printf("select communication type.\n");
				printf("please input 0 or 1(Wi-Fi/Bluetooth).\n");
				scanf_s("%d", &type);
				switch (type) {
				case 0:
					printf("communication type=Wi-Fi\n");
					communication_type = WIFI;
					break;
				case 1:
					printf("communication type=Bluetooth\n");
					communication_type = BLUETOOTH;
					break;
				default:
					printf("can't select communication type\n");
					break;
				}

			}
			break;
		case 'q':
		case 'Q':
			printf("exit program.\n");
			loop = 0;
			break;

		case '\n':
		case '\r':
		case '\0':
			err = 1;
			break;

		default:
			err = -2;
			break;
		}

		switch (err) {
		case -1:
			printf("Communication error.\n");
			break;
		case -2:
			printf("Unknow command.\n");
			break;
		case 0:
			printf("Command executed.\n");
			break;
		}

	}

	return 0;

}




/*
	bluetooth(シリアル通信)でナノローバーにメッセージを送信する
	引数:
	　char *send … 送信する文字列
	　char *recv … 受信した文字列を受け取るバッファへのポインタ。NULLの場合受信メッセージはコピーしない

	戻り値:
	　関数が成功したら0、失敗したら-1

*/
int send_COM(char *send, char *recv) {

	char recvbuf[BUFF_SIZE];
	DWORD wbytes, rbytes;
	HANDLE hCom = CreateFileA(COM, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (hCom == INVALID_HANDLE_VALUE) return -1;

	if (WriteFile(hCom, send, strnlen_s(send, BUFF_SIZE), &wbytes, NULL) == 0) {
		CloseHandle(hCom);
		return -1;
	}

	memset(recvbuf, 0, sizeof(recvbuf));

	if (recv != NULL) {
		int i = 0;
		while (i < BUFF_SIZE) {
			if (ReadFile(hCom, &recvbuf[i], 1, &rbytes, NULL) == 0) {
				CloseHandle(hCom);
				return -1;
			}
			if (rbytes > 0) {
				if (recvbuf[i] == '\n') break;
				i += rbytes;
			}
		}

		memcpy(recv, recvbuf, strnlen_s(recvbuf, BUFF_SIZE));

	}

	CloseHandle(hCom);
	return 0;
}


/*
	WiFi(HTTP POST)でナノローバーにメッセージを送信する
	引数:
	　char *json … 送信する文字列(json形式)
	　char *recvbuf … 受信した文字列を受け取るバッファへのポインタ。NULLの場合受信メッセージはコピーしない

	戻り値:
	　関数が成功したら0、失敗したら-1

*/
int HttpPost(char *json)
{
	char destination[] = IP_ADDRESS;
	unsigned short port = 80;
	char httppath[] = "/";
	char httphost[] = IP_ADDRESS;
	int sock;

	struct sockaddr_in dstAddr;

	char toSendText[BUFF_SIZE];
	char buf[BUFF_SIZE];
	int read_size;
	int length = strnlen_s(json, BUFF_SIZE);


	// winsockの初期化
	WSADATA data;
	WSAStartup(MAKEWORD(2, 0), &data);

	// sockaddr_in構造体の初期化
	memset(&dstAddr, 0, sizeof(dstAddr));
	dstAddr.sin_port = htons(port);
	dstAddr.sin_family = AF_INET;
	dstAddr.sin_addr.s_addr = inet_addr(destination);

	// ソケットの生成
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		WSACleanup();
		return -1;
	}

	// socketの接続
	if (connect(sock, (struct sockaddr *) &dstAddr, sizeof(dstAddr)) < 0) {
		closesocket(sock);
		WSACleanup();
		return -1;
	}

	// HTTP-headerの作成と送信
	sprintf_s(toSendText, "POST %s HTTP/1.1\r\n", httppath);
	send(sock, toSendText, strlen(toSendText), 0);

	sprintf_s(toSendText, "Host: %s:%d\r\n", httphost, port);
	send(sock, toSendText, strlen(toSendText), 0);

	sprintf_s(toSendText, "Content-Type: application/json; charset=utf-8\r\n");
	send(sock, toSendText, strlen(toSendText), 0);

	sprintf_s(toSendText, "Content-Length: %d\r\n", length);
	send(sock, toSendText, strlen(toSendText), 0);

	// bodyの送信
	send(sock, json, length, 0);


	//HTTP-Response・bodyの受信
	for (int i = 0; i < 2; i++) {
		Sleep(100);
		memset(buf, 0, sizeof(buf));
		read_size = recv(sock, buf, BUFF_SIZE, 0);
		if (read_size > 0) {
			printf("%s", buf);
		}
		else break;
	}

	//エンドアップ
	closesocket(sock);
	WSACleanup();

	return 0;
}

/*
	エンコーダ値を取得して画面に表示する
	戻り値:
	　関数が成功したら0、失敗したら-1

*/
int read_encoder() {

	switch (communication_type) {
	case BLUETOOTH:
		char recv[BUFF_SIZE];
		memset(recv, 0, sizeof(recv));
		if (send_COM("E\n", recv) < 0) return -1;
		printf(recv);
		break;
	case WIFI:
		return HttpPost("{\"command\":\"get_encoder\"}");
	default:
		return -1;
	}

	return 0;
}


/*
	指定のchのLEDの色を変更する
	引数:
	　int ch … 変更するLEDのch(0～8)
	　unsigned char r, unsigned char g, unsigned char b … 変更する色のR/G/Bの各要素(0～255)

	戻り値:
	　関数が成功したら0、失敗したら-1

*/
int set_LED_color(int ch, unsigned char r, unsigned char g, unsigned char b) {
	char send[BUFF_SIZE];


	switch (communication_type) {
	case BLUETOOTH:
		//int ch_bit = 1 << ch;
		sprintf_s(send, "L %04x %02x %02x %02x\n", (1 << ch), r, g, b);
		return send_COM(send, NULL);
	case WIFI:
		sprintf_s(send, "{\"command\":\"set_LEDcolor\",\"channel\":[%d],\"color\":{\"r\":%d,\"g\":%d,\"b\":%d}}", ch, r, g, b);
		return HttpPost(send);
	default:
		return -1;
	}


}



/*
	指定の速度で車輪を動かす
	引数:
	　int l, int r … 左/右の車輪速度(mm/sec単位)

	戻り値:
	　関数が成功したら0、失敗したら-1

*/
int set_wheel_speed(int l, int r) {
	char send[BUFF_SIZE];

	switch (communication_type) {
	case BLUETOOTH:
		sprintf_s(send, "S %08x %08x\n", l, r);
		return send_COM(send, NULL);
	case WIFI:
		sprintf_s(send, "{\"command\":\"set_speed\",\"left\":%d,\"right\":%d}", l, r);
		return HttpPost(send);
	default:
		return -1;
	}

}



/*
	エンコーダ値をクリアする

	戻り値:
	　関数が成功したら0、失敗したら-1

*/
int clear_encoder() {
	switch (communication_type) {
	case BLUETOOTH:
		return send_COM("C\n", NULL);
	case WIFI:
		return HttpPost("{\"command\":\"clear_encoder\"}");
	default:
		return -1;
	}
}



/*
	ペンを上げ下げする
	引数:
	　int is_up … 0で下げる、0以外で上げる

	戻り値:
	　関数が成功したら0、失敗したら-1

*/
int set_pen_updown(int is_up) {
	switch (communication_type) {
	case BLUETOOTH:
		if(is_up) return send_COM("P 01\n", NULL);
		else return send_COM("P 00\n", NULL);
	case WIFI:
		if (is_up) return HttpPost("{\"command\":\"pen_updown\",\"is_up\":true}");
		else return HttpPost("{\"command\":\"pen_updown\",\"is_up\":false}");
	default:
		return -1;
	}
}


