/*===============================================================================
 * Name        : wdt.h
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : WatchDog Timer definitions
 *===============================================================================*/
#ifndef _WDT_H_
#define _WDT_H_

/*----------------------------------------------------------------------
 * 関数のプロトタイプ宣言
 *----------------------------------------------------------------------*/
extern void wdtInit(void);
extern void wdtTimeout(unsigned long msec, void (*f)(void));
extern void wdtFeed(void);
extern void wdtFeedSysTick(unsigned long msec);
extern void wdtStop(void);
extern void wdtStart(void);

/*----------------------------------------------------------------------
 * タイムアウトフラグのクリア
 *----------------------------------------------------------------------*/
inline static void wdtClearTimeoutFlag(void) {
	// 18.7.1 Watchdog Mode register (WDMOD)
	// Bit 0 (WDEN) Watchdog enable bit
	// Bit 1 (WDRESET) Watchdog reset enable bit
	// Bit 2 (WDTOF) Watchdog time-out flag
	// Bit 7:4 (Reserved) should not write ones
	while (LPC_WDT->MOD & (1<<2)) {
		LPC_WDT->MOD &= (~(1<<2) & 0x0F);
	}
}

#ifdef	EXAMPLE
/*===============================================================================
 * ウォッチドッグタイマーの動作確認
 * - タイムアウト時の振る舞い確認
 * - exampleType
 *	1: メイン処理中でウォッチドッグを叩く
 *	2: SysTick割り込み中でウォッチドッグを叩く
 *===============================================================================*/
extern void wdtExample(int exampleType);
#endif // EXAMPLE

#endif // _WDT_H_
