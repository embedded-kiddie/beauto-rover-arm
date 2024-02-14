/*===============================================================================
 * Name        : sci.c
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : Pulse Width Demodulator functions using CT16B0, CT16B1
 *===============================================================================*/
// Cortex Microcontroller Software Interface Standard
#ifdef __USE_CMSIS
#include "LPC13xx.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "type.h"
#include "timer.h"
#include "ioport.h"
#include "sci.h"

#include "usb.h"
#include "usbhw.h"
#include "cdcuser.h"

/*--------------------------------------------------------------------------
 * CMSISのstdio系関数がリンクされる場合は、重複するシステムコールの定義を無効にする
 *--------------------------------------------------------------------------*/
#define	SYSTEM_CALL	1	// 有効化=1、無効化=0

/*----------------------------------------------------------------------
 * 待ち時間の設定
 * 1[msec]に設定すると、921600bpsで送信失敗が発生する
 *----------------------------------------------------------------------*/
#define	SCI_WAIT	5	// [msec]

/*----------------------------------------------------------------------
 * シリアル通信I/F - 初期化
 *----------------------------------------------------------------------*/
void SCI_INIT(void)
{
	// 7.4.18 IOCON_PIO0_6 (PIO0_6/USB_CONNECT/SCK)
	// Bit 2:0 FUNC
	// 0x0 = Selects function PIO0_6
	// 0x1 = Selects function USB_CONNECT
	// 0x2 = Selects function SCK0
	SetGpioBit(LPC_GPIO0, 6, 1);

	SciInit();
}

/*--------------------------------------------------------------------------
 * シリアル通信出力 - 書式指定付き出力
 *--------------------------------------------------------------------------*/
int SCI_PRINTF(const char *fmt, ...)
{
	int len = 0;
	/*static*/ char buf[SCI_BUF_SIZE];

	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	len = vsnprintf(buf, SCI_BUF_SIZE, fmt, arg_ptr); // ヌル文字を除く63文字でカット
	va_end(arg_ptr);

	// 送信
	SciStrTx((unsigned char*)buf, (unsigned char)len);

	// 次の送信まで間を空ける
	WAIT(SCI_WAIT);

	return len;
}

/*--------------------------------------------------------------------------
 * シリアル通信入力 - 1行分の文字列を読み取り、バッファに格納する
 *--------------------------------------------------------------------------*/
int SCI_GETS(char *buf, int len) {
	int n = 0;
	unsigned char c;

	// エスケープシーケンス - 1文字後退し、カーソル以降を消去する
	// ANSIエスケープシーケンス チートシート #Bash - Qiita
	// https://qiita.com/PruneMazui/items/8a023347772620025ad6
	unsigned char bs[] = "\b\e[0K";

	// エコーバックしながら1文字ずつ読み込む
	while (n < len - 1) {
		if (SciByteRx(&c)) {
			// 表示可能文字なら1文字格納
			if (!iscntrl(c) && isprint(c)) {
				SciByteTx(c);
				buf[n++] = c;
			}
			// リターンなら入力終了
			else if (c == '\r') {
				break;
			}
			// 他の制御コードなら後退
			else if (n > 0) {
				n--;
				SciStrTx(bs, sizeof(bs));
			}
		}
	}

	// ヌル文字で終端して返す
	buf[n] = 0;
	return n;
}

/*--------------------------------------------------------------------------
 * シリアル通信入力 - 書式指定付き入力（簡易版）
 *--------------------------------------------------------------------------*/
int SCI_SCANF(const char* fmt, ...) {
	int len = 1;
	char buf[SCI_BUF_SIZE];
	const char *p = fmt;

	SCI_GETS(buf, SCI_BUF_SIZE);

	va_list arg_ptr;
	va_start(arg_ptr, fmt);

	// '%' の次にアルファベットが出現するまで読み飛ばす
	while (*p && *p != '%'   ) p++;
	while (*p && !isalpha(*p)) p++;

	switch (*p) {
	  case 'd':
		sscanf(buf, fmt, va_arg(arg_ptr, int*));
		break;
	  case 'x':
		sscanf(buf, fmt, va_arg(arg_ptr, int*));
		break;
	  case 'c':
	  case 's':
		sscanf(buf, fmt, va_arg(arg_ptr, char*));
		break;
	  case 'f':
		sscanf(buf, fmt, va_arg(arg_ptr, double*));
		break;
	  default:
		len = 0;
	}

	va_end(arg_ptr);
	return len;
}

/*--------------------------------------------------------------------------
 * printf()、scanf()用システムコール
 *
 * 参考情報
 *
 * 1. Using printf() - NXP Community
 * https://community.nxp.com/t5/LPCXpresso-IDE-FAQs/Using-printf/m-p/474799
 *
 * 2. Switching the selected C library
 * https://community.nxp.com/t5/LPCXpresso-IDE-FAQs/Switching-the-selected-C-library/m-p/473669
 *
 * 3. LPC1114+CMSIS Libraryでprintfを使ってみた - 日曜技術者のメモ
 * https://se.hatenablog.jp/entry/2015/04/29/113130
 *
 * 4. libconfig-arm.h
 * C:\nxp\LPCXpresso_8.2.2_650\lpcxpresso\tools\redlib\include\
 *--------------------------------------------------------------------------*/
int __sys_istty(int handle) {
	return 1;
}

int __sys_flen(int handle) {
	return 0;
}

void __sys_appexit (void) {
}

int __sys_seek(int handle, int pos) {
	return 0;
}

/*--------------------------------------------------------------------------
 * CMSISのstdio系関数がリンクされる場合は、重複するシステムコールの定義を無効にする
 *--------------------------------------------------------------------------*/
#if	SYSTEM_CALL

int __sys_write(int iFileHandle, char *pcBuffer, int iLength) {
	// 送信
	SciStrTx((unsigned char*)pcBuffer, (unsigned char)iLength);

	// 次の送信まで間を空ける
	WAIT(SCI_WAIT);

	return 0;
}

int __sys_read(int iFileHandle, char *pcBuffer, int iLen) {
	// 1行分の文字列を読み取り、バッファに格納する
	SCI_GETS(pcBuffer, iLen);
	return 1;
}

int __sys_readc(void) {
	unsigned char c = 0;
	while (1) {
		if (SciByteRx(&c)) {
			return (int)c;
		}
	}
}

#endif

#ifdef	EXAMPLE
/*===============================================================================
 * USBシリアル通信の動作確認
 *	- ホストPCとUSBケーブルで接続し、ターミナルソフトを起動すること
 *	- 921600bpsまで動作確認済み
 *===============================================================================*/
#include <stdio.h>
#include "type.h"
#include "ioport.h"
#include "sci.h"

/*----------------------------------------------------------------------
 * 動作例1: エコーバック
 *----------------------------------------------------------------------*/
void SCI_EXAMPLE1(void) {
	while (1) {
		unsigned char data = 0;
		if (SciByteRx(&data)) {
			SciByteTx(data);
		}
	}
}

/*----------------------------------------------------------------------
 * 動作例2: printf(), scanf()
 *----------------------------------------------------------------------*/
void SCI_EXAMPLE2(void) {
	int i = 0;
	const char *txt = "0123456789012345678901234567890123456789012345678901234567890123456789\r\n";

	CDC_SetLineCoding();
	SCI_PRINTF("%s%s", "\e[2J", "\e[0;0H"); // 画面クリア＋原点に移動
	SCI_PRINTF("Baud rate   = %d\r\n", CDC_LineCoding.dwDTERate);
	SCI_PRINTF("Stop bit(s) = %d\r\n", CDC_LineCoding.bCharFormat);
	SCI_PRINTF("Parity type = %d\r\n", CDC_LineCoding.bParityType);
	SCI_PRINTF("Data bits   = %d\r\n", CDC_LineCoding.bDataBits);

	printf("Circle ratio = %7.4lf\r\n", (double)3.1415926);
	printf("Cyborg %03d is a Japanese famous animation.\r\n", 9);
	printf(txt); // ヌル文字に関係なく64文字でカット

	printf("%sPlease input a number: ", "\r\n");
	SCI_SCANF("%8d", &i); // scanf()は動作せず
	printf("%sYour input is '%d'.", "\r\n", i);

	while (1) {
		printf("\e[%d;%dH Timer = %10ld", 10, 30, TIMER_READ());
	}
}

/*--------------------------------------------------------------------------
 * USBシリアル通信の動作例
 *--------------------------------------------------------------------------*/
void SCI_EXAMPLE(int type) {
	TIMER_INIT();	// WAIT()
	PORT_INIT();	// SW_STANDBY()
	SCI_INIT();		// PIO0_3がUSB_VBUSと競合するため、LED1（橙）点灯せず

	SW_STANDBY();	// 通信の確立を確認し、SW1で動作を開始する
	LED(LED_ON);	// LED2（緑）のみ点灯

	switch (type) {
	  case 1:
		SCI_EXAMPLE1();
		break;

	  case 2:
	  default:
		SCI_EXAMPLE2();
		break;
	}
}
#endif // EXAMPLE
