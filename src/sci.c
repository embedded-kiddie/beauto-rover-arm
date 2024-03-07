/*===============================================================================
 * Name        : sci.c
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : Serial Communication Interface using USB CDC
 *===============================================================================*/
// Cortex Microcontroller Software Interface Standard
#ifdef __USE_CMSIS
#include "LPC13xx.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "type.h"
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
 * - 連続送出時、5[msec]の設定だと、受信側で取りこぼしが発生する
 *----------------------------------------------------------------------*/
#define	SCI_WAIT	(2000UL * 10)		// [10msec]
extern void delay	(uint32_t length);	// defined in usbhw.c

/*----------------------------------------------------------------------
 * シリアル通信I/F - 初期化
 * - Tera Term で 921600bps まで動作確認済み
 *----------------------------------------------------------------------*/
void sciInit(void) {
	SciInit(); // defined in cdcuser.c
}

/*----------------------------------------------------------------------
 * シリアル通信I/F - ホストから '\r'（リターンキー）が送られてくるまで待つ
 * - 使用方法
 *	sciInit();		// Tera Term の [未接続] が消えたら
 *	sciWaitKey();	// Tera Term から '\r'（リターンキー）を送出する
 *----------------------------------------------------------------------*/
int sciWaitKey(void) {
	unsigned char data = 0;
	while (1) {
		if (SciByteRx(&data) && data == '\r') {
			return TRUE;
		}
	}
}

/*--------------------------------------------------------------------------
 * シリアル通信出力 - 書式指定付き出力
 * - ネイティブの printf() には待ち時間が入っていないので、本関数の使用を推奨する
 *--------------------------------------------------------------------------*/
int sciPrintf(const char* restrict fmt, ...) {
	int len = 0;
	/*static*/ char buf[SCI_BUF_SIZE];

	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	len = vsnprintf(buf, SCI_BUF_SIZE, fmt, arg_ptr); // ヌル文字を含む64文字でカット
	va_end(arg_ptr);

	// 送信
	SciStrTx((unsigned char*)buf, (unsigned char)len);

	// 次の送信まで間を空ける
	delay(SCI_WAIT);

	return len;
}

/*--------------------------------------------------------------------------
 * シリアル通信入力 - 1行分の文字列を読み取り、バッファに格納する
 *--------------------------------------------------------------------------*/
int sciGets(char *buf, int len) {
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
 * - 制限事項
 * 	・１回に入力できる変数は１つのみ
 * 	  NG例： sciScanf("%d%f", &i, &f);
 * 	・代入抑止文字 '*' は使用不可
 * 	・最大フィールド幅（正の10進数で指定される文字数）は指定可能
 * 	・型指定子は 'c'、's'、'd'、'x'、'f'、'ld'、'lx'、'lf' のみ可能
 *--------------------------------------------------------------------------*/
int sciScanf(const char* restrict fmt, ...) {
	int len = 1;
	char buf[SCI_BUF_SIZE];
	const char *p = fmt;

	sciGets(buf, SCI_BUF_SIZE);

	va_list arg_ptr;
	va_start(arg_ptr, fmt);

	// '%' の次にアルファベットが出現するまで読み飛ばす
	while (*p && *p != '%'   ) p++;
	while (*p && !isalpha(*p)) p++;

	switch (*p) {
	  case 'd':
	  case 'x':
		sscanf(buf, fmt, va_arg(arg_ptr, int*));
		break;
	  case 'c':
	  case 's':
		sscanf(buf, fmt, va_arg(arg_ptr, char*));
		break;
	  case 'f':
		sscanf(buf, fmt, va_arg(arg_ptr, float*));
		break;
	  case 'l':
		switch(*(p+1)) {
		  case 'd':
		  case 'x':
			sscanf(buf, fmt, va_arg(arg_ptr, long*));
			break;
		  case 'f':
			sscanf(buf, fmt, va_arg(arg_ptr, double*));
			break;
		  default:
			len = 0;
		}
		break;
	  default:
		len = 0;
	}

	va_end(arg_ptr);
	return len;
}

#if	FALSE
/*--------------------------------------------------------------------------
 * シリアル通信入力 - scanf()
 * - redlibで定義済みだが、オーバーライドするシステムコールが不明なため使えない
 *--------------------------------------------------------------------------*/
int scanf(const char* restrict fmt, ...) {
	int len;

	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	len = sciScanf(fmt, arg_ptr);
	va_end(arg_ptr);

	return len;
}
#endif // FALSE

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
	delay(SCI_WAIT);

	return 0;
}

int __sys_read(int iFileHandle, char *pcBuffer, int iLen) {
	// 1行分の文字列を読み取り、バッファに格納する
	sciGets(pcBuffer, iLen);
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
 * - ホストPCとUSBケーブルで接続し、ターミナルソフトを起動すること
 * - 921600bpsまで動作確認済み
 *===============================================================================*/
#include <stdio.h>
#include "timer.h"
#include "gpio.h"
#include "sci.h"

/*----------------------------------------------------------------------
 * 動作例1: エコーバック
 *----------------------------------------------------------------------*/
void sciExample1(void) {
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
void sciExample2(void) {
	double f = 0.0;
	const char *txt = "0123456789012345678901234567890123456789012345678901234567890123456789\r\n";

	CDC_SetLineCoding();
	sciPrintf("%s%s", "\e[2J", "\e[0;0H"); // 画面クリア＋原点に移動
	sciPrintf("Baud rate   = %d\r\n", CDC_LineCoding.dwDTERate);
	sciPrintf("Stop bit(s) = %d\r\n", CDC_LineCoding.bCharFormat);
	sciPrintf("Parity type = %d\r\n", CDC_LineCoding.bParityType);
	sciPrintf("Data bits   = %d\r\n", CDC_LineCoding.bDataBits);

	printf("Circle ratio = %7.4lf\r\n", (double)3.1415926);
	printf("Cyborg %03d is a Japanese famous animation.\r\n", 9);
	printf(txt); // ヌル文字に関係なく64文字でカット

	printf("%sPlease input a number: ", "\r\n");
	sciScanf("%8lf", &f); // scanf()は動作せず
	printf("%sYour input is '%f'.", "\r\n", f);

	while (1) {
		printf("\e[%d;%dH Timer = %10ld", 10, 30, timerRead());
	}
}

/*--------------------------------------------------------------------------
 * USBシリアル通信の動作例
 * - exampleType
 *	1: エコーバック
 *	2: printf(), scanf()
 *--------------------------------------------------------------------------*/
void sciExample(int exampleType) {
	timerInit();	// timerWait()
	gpioInit();		// swStandby()
	sciInit();		// PIO0_3がUSB_VBUSと競合するため、LED1（橙）点灯せず

	swStandby();	// 通信の確立を確認し、SW1で動作を開始する
	ledOn(LED_ON);	// LED2（緑）のみ点灯

	switch (exampleType) {
	  case 1:
		sciExample1();
		break;

	  case 2:
	  default:
		sciExample2();
		break;
	}
}
#endif // EXAMPLE
