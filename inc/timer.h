/*===============================================================================
 * Name        : timer.h
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : Hardware / Software timer definitions
 *===============================================================================*/
#ifndef _TIMER_H_
#define _TIMER_H_

/*----------------------------------------------------------------------
 * 1[msec]を刻むプリスケールの設定
 * 16.8.4 PreScale Register (TMR32B1PR)
 *----------------------------------------------------------------------*/
#define	TIMER_PRE_SCALE		72000	// 1[KHz] = 0.001[sec]

/*----------------------------------------------------------------------
 * 関数のプロトタイプ宣言、関数へのNULLポインタ
 *----------------------------------------------------------------------*/
extern void timerInit(void);
extern unsigned long timerRead(void);
extern void timerWakeup(unsigned long msec, void (*f)(void));

/*----------------------------------------------------------------------
 * 指定時間[msec]の待機、待機のキャンセル
 * timerInit()が実行された場合は Hardware timer を使用する
 *----------------------------------------------------------------------*/
extern void timerWait(unsigned long msec);
extern void timerWaitCancel(void);

/*----------------------------------------------------------------------
 * 長時間（約49日）の待機
 * 使用例：
 *	swWatch(timerWaitCancel);
 *	timerWait(WAIT_FOREVER);
 *----------------------------------------------------------------------*/
#define	WAIT_FOREVER	(unsigned long)(-1)

#endif /* _TIMER_H_ */
