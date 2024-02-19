/*===============================================================================
 * Name        : pmu.h
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : Power Management Unit definitions
 *===============================================================================*/
#ifndef	_PMU_H_
#define	_PMU_H_

/*----------------------------------------------------------------------
 * 電力マネジメントモード
 *----------------------------------------------------------------------*/
#define	PMU_ACTIVE_MODE		0	// 3.9.1 Active mode
#define	PMU_SLEEP_MODE		1	// 3.9.2 Sleep mode
#define	PMU_DEEP_SLEEP		2	// 3.9.3 Deep-sleep mode
#define	PMU_POWER_DOWN		3	// 3.9.4 Deep power-down mode

/*----------------------------------------------------------------------
 * 関数のプロトタイプ宣言
 *----------------------------------------------------------------------*/
extern void PMU_WAKEUP(unsigned long msec);
extern unsigned int PMU_SLEEP(int mode);

#ifdef	EXAMPLE
/*===============================================================================
 * パワーマネジメントユニットの動作確認
 * - Active/Sleep/Deep-sleep/Deep Power-down 各モードの動作例
 * - exampleType
 *	1: Active mode　でのスリープ
 *	2: Sleep/Deep-sleep/Deep Power-down
 *===============================================================================*/
extern void PMU_EXAMPLE(int exampleType);
#endif // EXAMPLE

#endif // _PMU_H_
