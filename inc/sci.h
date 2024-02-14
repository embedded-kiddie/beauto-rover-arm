/*===============================================================================
 * Name        : sci.h
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : Pulse Width Demodulator functions using CT16B0, CT16B1
 *===============================================================================*/
#ifndef	_SCI_H_
#define	_SCI_H_

#include "usb.h"		// USB_SETUP_PACKET
#include "cdc.h"		// CDC_LINE_CODING
#include "cdcuser.h"	// SciByteRx(), SciByteTx(), SciStrTx()

/*----------------------------------------------------------------------
 * 関数のプロトタイプ宣言
 *----------------------------------------------------------------------*/
extern void SCI_INIT(void);
extern int SCI_PRINTF(const char *fmt, ...);
extern int SCI_SCANF(const char* fmt, ...);
extern int SCI_GETS(char *buf, int len);
extern int printf(const char *fmt, ...);

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
 *	- ホストPCとUSBケーブルで接続し、ターミナルソフトを起動すること
 *	- 921600bpsまで動作確認済み
 *===============================================================================*/
extern void SCI_EXAMPLE(int type);
#endif // EXAMPLE

#endif // _SCI_H_
