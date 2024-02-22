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

/*----------------------------------------------------------------------
 * パワーマネジメント対象を表すマスクの定義
 *----------------------------------------------------------------------*/
// 3.5.45 Deep-sleep mode configuration register (PDSLEEPCFG)
#define PDSLEEPCFG_BOD		(1<<3)	// BOD Powered down = ~0x0FF7
#define PDSLEEPCFG_WDTOSC	(1<<6)	// WDT oscillator Powered down = ~0x0FBF
#define PDSLEEPCFG_FIXEDVAL	((7<<0) | (3<<4) | (0x1F<<7)) // 0xFB7
#define	PDSLEEPCFG_MASK		((1<<3) | (1<<6)) // ~0x0FB7

// 3.5.46 Wake-up configuration register (PDAWAKECFG)
#define	PDAWAKECFG_IRCOUT	(1<<0)
#define	PDAWAKECFG_IRC		(1<<1)
#define	PDAWAKECFG_FLASH	(1<<2)
#define	PDAWAKECFG_BOD		(1<<3)
#define	PDAWAKECFG_ADC		(1<<4)
#define	PDAWAKECFG_SYSOSC	(1<<5)
#define	PDAWAKECFG_WDTOSC	(1<<6)
#define	PDAWAKECFG_SYSPLL	(1<<7)
#define	PDAWAKECFG_USBPLL	(1<<8)
#define	PDAWAKECFG_USBPAD	(1<<10)
#define	PDAWAKECFG_FIXEDVAL	((0<<9)|(1<<11)|(1<<12)|(7<<13)) // 0xE800
#define	PDAWAKECFG_MASK		((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<8)|(1<<10)) // 0x05FF

// 3.5.47 Power-down configuration register (PDRUNCFG)
#define	PDRUNCFG_IRCOUT		(1<<0)
#define	PDRUNCFG_IRC		(1<<1)
#define	PDRUNCFG_FLASH		(1<<2)
#define	PDRUNCFG_BOD		(1<<3)
#define	PDRUNCFG_ADC		(1<<4)
#define	PDRUNCFG_SYSOSC		(1<<5)
#define	PDRUNCFG_WDTOSC		(1<<6)
#define	PDRUNCFG_SYSPLL		(1<<7)
#define	PDRUNCFG_USBPLL		(1<<8)
#define	PDRUNCFG_USBPAD		(1<<10)
#define	PDRUNCFG_FIXEDVAL	((0<<9)|(1<<11)|(1<<12)|(7<<13)) // 0xE800
#define	PDRUNCFG_MASK		((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<8)|(1<<10)) // 0x05FF

/*----------------------------------------------------------------------
 * パワーマネジメント対象を設定する関数
 *----------------------------------------------------------------------*/
extern void SetPowerDownSleep(unsigned long mask);
extern void SetPowerDownAwake(unsigned long mask);
extern void DelPowerDownRun(unsigned long mask);
extern void AddPowerDownRun(unsigned long mask);

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
