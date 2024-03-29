/*===============================================================================
 * Name        : sci.h
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : Serial Communication Interface using USB CDC
 *===============================================================================*/
#ifndef	_SCI_H_
#define	_SCI_H_

#include "usb.h"		// USB_SETUP_PACKET
#include "cdc.h"		// CDC_LINE_CODING
#include "cdcuser.h"	// SciByteRx(), SciByteTx(), SciStrTx()

/*----------------------------------------------------------------------
 * 関数のプロトタイプ宣言
 *----------------------------------------------------------------------*/
extern void sciInit(void);
extern int sciWaitKey(unsigned char key);
extern int sciPrintf(const char* restrict fmt, ...);
extern int sciScanf(const char* restrict fmt, ...);
extern int sciGets(char *buf, int len);
extern int printf(const char* restrict fmt, ...);

/*----------------------------------------------------------------------
 * 通信設定を取得する関数とその変数（cdcuser.c）
 *----------------------------------------------------------------------*/
extern uint32_t CDC_SetLineCoding (void);
extern CDC_LINE_CODING CDC_LineCoding;

/*----------------------------------------------------------------------
 * vsnprintf()用内部バッファのサイズ （ヌル文字を含む）
 *----------------------------------------------------------------------*/
#define	SCI_BUF_SIZE	64	// SciStrTx()のサイズ制限と同じ

#ifdef	EXAMPLE
/*===============================================================================
 * USBシリアル通信の動作確認
 * - ホストPCとUSBケーブルで接続し、ターミナルソフトを起動すること
 * - 921600bpsまで動作確認済み
 * - exampleType
 *	1: エコーバック
 *	2: printf(), scanf()
 *===============================================================================*/
extern void sciExample(int exampleType);
#endif // EXAMPLE

#endif // _SCI_H_
